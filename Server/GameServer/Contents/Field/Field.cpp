#include "pch.h"
#include "Field.h"

#include <filesystem>
#include <utility>

#include "FieldManager.h"
#include "Contents/Player.h"
#include "GameSession.h"
#include "LogManager.h"
#include "Packet/ServerPacketHandler.h"
#include "Detour/Include/DetourNavMeshQuery.h"


Field::Field(uint64 id, const FieldData* fieldData) : _navMesh(fieldData->NavMesh()), _id(id), _fieldData(fieldData)
{
	_navQuery = dtAllocNavMeshQuery();
	dtStatus Status = _navQuery->init(_navMesh, 2048);
	if (dtStatusFailed(Status))
	{
		dtFreeNavMeshQuery(_navQuery);
	}
}

Field::~Field()
{
	LOG_DEBUG(Default, "FieldInstance : {} is Destroyed", _id);
	dtFreeNavMeshQuery(_navQuery);
}

void Field::Init()
{
	JobRef job = make_shared<Job>([weakSelf = weak_from_this()]() {
		auto self = weakSelf.lock();
		if(self == nullptr) return;

		self->UpdatePlayerPosition();
	});

	LJobTimer.Reserve(500, GetJobQueue(), job);
}

void Field::EnterPlayer(weak_ptr<PlayerCharacter> player)
{
	_destroyToken.fetch_add(1);
	DoAsync([self = shared_from_this(), playerWeak = std::move(player)]()
	{
		auto player = playerWeak.lock();
		if(!player) return;

		self->_currentPlayerCount.fetch_add(1);
		self->_players.insert(player);

		player->Transform()->SetPos( self->_fieldData->PlayerStarts()[0]);
		player->SetField(self);

		{
			Protocol::SC_ENTER_FIELD packet;
			player->GetObjectInfo(packet.mutable_my_info()->mutable_object_info());
			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
			auto session = player->GetSession();
			if (session) session->Send(sendBuffer);
		}

		// 주변 유저에게 새로 들어온 플레이어 스폰
		{
			Protocol::SC_SPAWN_PLAYER packet;
			Protocol::PlayerInfo* playerInfo = packet.add_info();
			player->GetObjectInfo(playerInfo->mutable_object_info());

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
			self->BroadCast(sendBuffer, player);
		}

		// 새로 들어온 플레이어에게 주변 유저 스폰
		{
			if(self->_players.empty()) return;
			Protocol::SC_SPAWN_PLAYER packet;
			vector<Protocol::SC_MOVE_PATH> movePackets;
			movePackets.reserve(self->_players.size());
			for (const shared_ptr<PlayerCharacter>& other : self->_players)
			{
				if (other == player) continue;
				other->GetObjectInfo(packet.add_info()->mutable_object_info());
				if(other->IsMoving())
				{
					Protocol::SC_MOVE_PATH movePacket;
					movePacket.set_start_server_tick(other->GetMoveStartTime());
					movePacket.set_object_id(other->GetId());
					const auto& waypoints = other->GetWaypoints();
					for(const auto& waypoint : waypoints)
					{
						auto* newWayPoint = movePacket.add_waypoints();
						newWayPoint->set_x(waypoint.x);
						newWayPoint->set_y(waypoint.y);
						newWayPoint->set_z(waypoint.z);
					}
					movePackets.push_back(std::move(movePacket));
				}
			}

			if (packet.info_size() == 0) return;
			auto session = player->GetSession();
			if (!session) return;

			{
				SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
				session->Send(sendBuffer);
			}

			for(auto& movePacket : movePackets)
			{
				SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePacket);
				session->Send(sendBuffer);
			}

		}
	});
}

void Field::BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except)
{
	DoAsync([self = shared_from_this(), sendBuffer = std::move(sendBuffer), except]()
	{
		vector<SessionRef> sessions;
		sessions.reserve(self->_players.size());

		for (auto& player : self->_players)
		{
			if (except == player) continue;
			auto session = player->GetSession();
			if (session) sessions.push_back(session);
		}

		for (auto& session : sessions)
		{
			session->Send(sendBuffer);
		}
	});
}

void Field::PlayerRequestMove(weak_ptr<PlayerCharacter> player, const Protocol::Vector3& dest)
{
	DoAsync([self = shared_from_this(), player = std::move(player), dest]()
	{
		auto playerRef = player.lock();
		if(playerRef == nullptr) return;
		if(!self->_players.contains(playerRef))
		{
			LOG_ERROR(Default, "PlayerRequestMove Error");
			return;
		}

		Vector3 startPos = playerRef->GetCurrentPosition(GetTickCount64());
		Vector3 endPos(dest);

		std::vector<Vector3> serverWaypoints;
		self->FindPath(startPos, endPos, serverWaypoints);

		Protocol::SC_MOVE_PATH pkt;
		pkt.set_start_server_tick(GetTickCount64());
		pkt.set_object_id(playerRef->GetId());

		for (int i = 0; i < serverWaypoints.size(); i++)
		{
			LOG_DEBUG(PathFind, "[{}] : [{}, {}, {}]", i, serverWaypoints[i].x, serverWaypoints[i].y, serverWaypoints[i].z);

			auto* waypoint = pkt.add_waypoints();
			waypoint->set_x(serverWaypoints[i].x);
			waypoint->set_y(serverWaypoints[i].y);
			waypoint->set_z(serverWaypoints[i].z);

			serverWaypoints[i] = Vector3(*waypoint); // Ensure serverWaypoints matches exact packet format if needed, but they are already identical.
		}
		playerRef->SetMoveInfo(std::move(serverWaypoints), GetTickCount64(), 500.0f);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		self->BroadCast(sendBuffer);
	});
}

void Field::LeavePlayer(shared_ptr<PlayerCharacter> player, shared_ptr<Field> nextField)
{
	DoAsync([self = shared_from_this(), player = std::move(player), nextField = std::move(nextField)]() {
		player->SetField(nullptr);

		Protocol::SC_DESPAWN_PLAYER pkt;
		pkt.set_player_id(player->GetId());
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		self->BroadCast(sendBuffer);

		self->_players.erase(player);
		self->_currentPlayerCount.fetch_sub(1);

		if(self->_players.empty())
		{
			JobRef job = make_shared<Job>([self, fieldId = self->_id, Token = self->_destroyToken.load()]()
			{
				if(self->_destroyToken.load() != Token) return;
				GFieldManager.Destroy(fieldId);
			});
			LJobTimer.Reserve(10000, self->GetJobQueue(), job);
		}

		if(!nextField) return;
		nextField->EnterPlayer(player);
	});
}

void Field::UpdatePlayerPosition()
{
	//LOG_DEBUG(FieldInstance, "UpdatePlayerPosition");

	uint64 now = GetTickCount64();
	for(auto& player : _players)
	{
		if(player->IsMoving())
		{
			player->Transform()->SetPos(player->GetCurrentPosition(now));
		}
	}

	JobRef job = make_shared<Job>([weakSelf = weak_from_this()]() {
		auto self = weakSelf.lock();
		if(self == nullptr) return;

		self->UpdatePlayerPosition();
	});

	LJobTimer.Reserve(500, GetJobQueue(), job);
}

void Field::FindPath(const Vector3& startPos, const Vector3& endPos, OUT std::vector<Vector3>& pathResult)
{
	auto start = std::chrono::high_resolution_clock::now();
	dtQueryFilter filter;
	filter.setIncludeFlags(0xffff);
	filter.setExcludeFlags(0);

	float extents[3] = { 0.5f, 1.0f, 0.5f };

	dtPolyRef StartPolyRef = 0;
	dtPolyRef EndPolyRef = 0;
	
	// Convert Engine coordinates (cm) to Detour coordinates (m)
	float startPt[3] { (float)startPos.x / 100.0f, (float)startPos.z / 100.0f, (float)startPos.y / 100.0f };
	float endPt[3] = { (float)endPos.x / 100.0f, (float)endPos.z / 100.0f, (float)endPos.y / 100.0f };

	float StartNearestPt[3];
	float EndNearestPt[3];

	_navQuery->findNearestPoly(startPt, extents, &filter, &StartPolyRef, StartNearestPt);
	_navQuery->findNearestPoly(endPt, extents, &filter, &EndPolyRef, EndNearestPt);

	if (!StartPolyRef || !EndPolyRef)
	{
		LOG_DEBUG(PathFind, "Failed to find start or end polygon on NavMesh!");
		return;
	}

	static constexpr int MAX_PATH_POLYS = 2048;
	dtPolyRef path[MAX_PATH_POLYS];
	int pathCount = 0;

	_navQuery->findPath(StartPolyRef, EndPolyRef, StartNearestPt, EndNearestPt, &filter, path, &pathCount, MAX_PATH_POLYS);

	if (pathCount == 0)
	{
		return;
	}

	// (D) 실제 이동 좌표 구하기 (String Pulling)
	unsigned char straightPathFlags[MAX_PATH_POLYS];
	dtPolyRef straightPathRefs[MAX_PATH_POLYS];
	float straightPath[MAX_PATH_POLYS * 3];
	int straightPathCount = 0;

	_navQuery->findStraightPath(StartNearestPt, EndNearestPt, path, pathCount, straightPath, straightPathFlags, straightPathRefs, &straightPathCount, MAX_PATH_POLYS);
	
	if(straightPathCount > 0)
	{
		LOG_DEBUG(PathFind, "Found Straight Path! Points: {}", straightPathCount);
		for (int i = 0; i < straightPathCount; ++i)
		{
			// Detour: X, Y, Z (m) -> Engine: X, Z, Y (cm)
			pathResult.emplace_back(straightPath[i*3] * 100.0f, straightPath[i*3+2] * 100.0f, straightPath[i*3+1] * 100.0f);
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	LOG_DEBUG(PathFind, "Execution Time : {}us", duration);
}
