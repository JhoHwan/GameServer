#include "pch.h"
#include "Player.h"

#include <utility>
#include "GameSession.h"
#include "LogManager.h"
#include "Field/Field.h"
#include "Field/FieldManager.h"
#include "Packet/ServerPacketHandler.h"
#include "Util/Time.h"

PlayerCharacter::PlayerCharacter(weak_ptr<GameSession> session) : _sessionRef(std::move(session))
{

}

PlayerCharacter::~PlayerCharacter()
{
	LOG_DEBUG(Default, "PlayerCharacter::~PlayerCharacter()");
}

void PlayerCharacter::Init()
{
	GameObject::Init();

	LOG_DEBUG(Default, "Player[{}] Created", GetInstanceID());

	auto session = GetSession();
	auto player = GetPlayerRef();
	if (session) session->SetPlayer(player);
}

void PlayerCharacter::OnSpawn()
{
	GameObject::OnSpawn();

	shared_ptr<Field> field = GetField();
	if(!field) return;

	auto self = GetPlayerRef();
	auto session = GetSession();
	if(!session) return;

	field->EnterPlayer(self);
	{
		Protocol::SC_ENTER_FIELD packet;
		GetObjectInfo(packet.mutable_my_info()->mutable_object_info());
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
		session->SendPacket(sendBuffer);
	}

	// 주변 유저에게 새로 들어온 플레이어 스폰
	{
		Protocol::SC_SPAWN_PLAYER packet;
		Protocol::PlayerInfo* playerInfo = packet.add_info();
		GetObjectInfo(playerInfo->mutable_object_info());

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
		field->BroadCast(sendBuffer, self);
	}

	// 새로 들어온 플레이어에게 주변 유저 스폰
	{
		const auto& players = field->GetPlayers();
		if(players.size() <= 1) return;

		Protocol::SC_SPAWN_PLAYER packet;
		vector<Protocol::SC_MOVE_PATH> movePackets;
		movePackets.reserve(players.size());
		for (const shared_ptr<PlayerCharacter>& other : players)
		{
			if (other == self) continue;
			other->GetObjectInfo(packet.add_info()->mutable_object_info());
			if(other->IsMoving())
			{
				Protocol::SC_MOVE_PATH movePacket;
				movePacket.set_object_id(other->GetId());
				movePacket.set_start_server_tick(other->GetMoveStartTime());

				const auto& waypoints = other->GetWaypoints();
				const auto& arrivalTimes = other->GetArrivalTimes();

				for (size_t i = 0; i < waypoints.size(); ++i)
				{
					auto* wp = movePacket.add_waypoints();
					wp->mutable_pos()->CopyFrom(waypoints[i].ToProto());

					auto offset = static_cast<uint32>(arrivalTimes[i] - other->GetMoveStartTime());
					wp->set_arrival_offset_ms(offset);
				}

				if (movePacket.waypoints_size() > 0)
				{
					movePackets.push_back(std::move(movePacket));
				}
			}
		}
		if (packet.info_size() == 0) return;

		session->SendPacket(ServerPacketHandler::MakeSendBuffer(packet));
		for(auto& movePacket : movePackets)
		{
			session->SendPacket(ServerPacketHandler::MakeSendBuffer(movePacket));
		}
	}
}

void PlayerCharacter::OnDespawn()
{
	GameObject::OnDespawn();
	auto session = GetSession();
	if(session) session->SetPlayer(nullptr);

	auto self = GetPlayerRef();
	auto field = GetField();
	if(!field) return;

	Protocol::SC_DESPAWN_PLAYER pkt;
	pkt.set_player_id(GetId());
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	field->BroadCast(sendBuffer);

	field->LeavePlayer(self);
}

void PlayerCharacter::HandleMoveRequest(const Protocol::Vector3& dest, uint64 startServerTick)
{
	constexpr int32 MOVE_REQUEST_MIN_INTERVAL = 500;
	constexpr float MOVE_REQUEST_MIN_DIST = 300.0f;

	auto& time = GetMoveStartTime();
	if(IsMoving() && startServerTick - time < MOVE_REQUEST_MIN_INTERVAL)
	{
		if(Vector3::Dist2D(dest, GetDestinationPosition()) <= MOVE_REQUEST_MIN_DIST)
		{
			return;
		}
	}

	GetField()->HandleRequestMove(GetPlayerRef(), dest, startServerTick);
}

void PlayerCharacter::SetMoveInfo(vector<Vector3> wayPoints, vector<uint64> moveArrivalTime, uint64 moveStartTime)
{
	_moveWaypoints = wayPoints;
	_moveStartTime = moveStartTime;
	_isMoving = true;
	_moveArrivalTimes = moveArrivalTime;

	JobRef job = make_shared<Job>([weakGameObject = weak_from_this(), moveToken = _moveStartTime]()
	{
		auto gameObject = weakGameObject.lock();
		if (!gameObject) return;

		auto self = static_pointer_cast<PlayerCharacter>(gameObject);
		if(moveToken != self->_moveStartTime) return;

		auto arrivalPos = self->_moveWaypoints.back();
		LOG_INFO(PathFind, "Player {}{} Arrive [{}, {}, {}]", self->GetSubID(), self->GetInstanceID(), arrivalPos.x, arrivalPos.y, arrivalPos.z);

		if(self->_isMoving)
		{
			self->_isMoving = false;
			self->Transform()->SetPos(arrivalPos);
		}
	});

	LJobTimer.Reserve(_moveArrivalTimes.back() - moveStartTime, GetField()->GetJobQueue(), job);
}

Vector3 PlayerCharacter::GetCurrentPosition(uint64 now) const
{
	if(!_isMoving || _moveWaypoints.empty() || now <= _moveStartTime) return Transform()->GetPos();
	if(now >= _moveArrivalTimes.back()) return _moveWaypoints.back();

	for (size_t i = 1; i < _moveArrivalTimes.size(); ++i)
	{
		if (now <= _moveArrivalTimes[i])
		{
			uint64 segmentStartTime = _moveArrivalTimes[i - 1];
			uint64 segmentEndTime = _moveArrivalTimes[i];

			float ratio = 0.0f;
			if (segmentEndTime > segmentStartTime)
			{
				ratio = static_cast<float>(now - segmentStartTime) / static_cast<float>(segmentEndTime - segmentStartTime);
			}

			Vector3 start = _moveWaypoints[i - 1];
			Vector3 end = _moveWaypoints[i];

			Vector3 diff = end - start;
			Vector3 currentPos = start + (diff * ratio);
			Transform()->SetPos(currentPos);
			return currentPos;
		}
	}

	Vector3 finalPos = _moveWaypoints.back();
	return finalPos;
}
