#include "pch.h"
#include "Field.h"

#include <utility>
#include "Contents/Player.h"
#include "GameSession.h"
#include "LogManager.h"
#include "Packet/ServerPacketHandler.h"

FieldManager& GFieldManager = FieldManager::Instance();

void FieldManager::Init()
{
	auto navMesh = NavMeshLoader::LoadNavMeshFromBin("Test.bin");
	if(navMesh == nullptr) return;

	_navMesh[0] = navMesh;
}

void FieldManager::Create(uint16 fieldId)
{
	{
		WRITE_LOCK;
		auto fieldIt = _navMesh.find(fieldId);
		if (fieldIt == _navMesh.end() || fieldIt->second == nullptr) return;
		auto field = fieldIt->second;

		_fields[fieldId] = make_shared<FieldInstance>(fieldId, field);
	}

	LOG_INFO("FieldManager : {} is Created", fieldId);
}

shared_ptr<FieldInstance> FieldManager::GetField(uint16 fieldId)
{
	shared_ptr<FieldInstance> field = nullptr;
	{
		READ_LOCK;
		auto fieldIt = _fields.find(fieldId);
		field = fieldIt == _fields.end() ? nullptr : fieldIt->second;
	}

	if (field == nullptr)
	{
		Create(fieldId);
	}

	field = _fields[fieldId];
	return field;
}

FieldInstance::FieldInstance(uint16 id, dtNavMesh* navMesh) : _id(id), _navMesh(navMesh)
{
	_navQuery = dtAllocNavMeshQuery();
	dtStatus Status = _navQuery->init(_navMesh, 2048);
	if (dtStatusFailed(Status))
	{
		dtFreeNavMeshQuery(_navQuery);
	}
}

void FieldInstance::EnterPlayer(shared_ptr<PlayerCharacter> player)
{
	DoAsync([this, player = std::move(player)]()
	{
		_players.insert(player);
		player->Transform()->SetPos({0, 0, 308.4});
		player->SetField(shared_from_this());

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
			Protocol::ObjectInfo* objInfo = playerInfo->mutable_object_info();
			player->GetObjectInfo(objInfo);

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
			BroadCast(sendBuffer, player);
		}

		// 새로 들어온 플레이어에게 주변 유저 스폰
		{
			Protocol::SC_SPAWN_PLAYER packet;
			{
				for (const auto& other : _players)
				{
					if (other == player) continue;
					other->GetObjectInfo(packet.add_info()->mutable_object_info());
				}
			}

			if (packet.info_size() > 0)
			{
				SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
				auto session = player->GetSession();
				if (session) session->Send(sendBuffer);
			}
		}
	});
}

void FieldInstance::BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except)
{
	DoAsync([this, sendBuffer = std::move(sendBuffer), except]()
	{
		vector<SessionRef> sessions;
		sessions.reserve(_players.size());

		for (auto& player : _players)
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

void FieldInstance::PlayerRequestMove(weak_ptr<PlayerCharacter> player, const Protocol::Vector3& dest)
{
	DoAsync([this, player = std::move(player), dest]()
	{
		auto playerRef = player.lock();
		if(playerRef == nullptr) return;
		if(_players.find(playerRef) == _players.end())
		{
			LOG_ERROR("PlayerRequestMove Error");
			return;
		}

		dtReal startPos[3] {playerRef->Transform()->GetPos().x, playerRef->Transform()->GetPos().z, playerRef->Transform()->GetPos().y};
		dtReal endPos[3] {dest.x(), dest.z(), dest.y()};

		dtQueryResult result;
		FindPath(startPos, endPos, result);

		std::vector<Vector3> serverWaypoints;
		Protocol::SC_MOVE_PATH pkt;
		pkt.set_object_id(playerRef->GetId());

		for (int i = 0; i < result.size(); i++)
		{
			auto* pos = result.getPos(i);
			LOG_DEBUG("[{}] : [{}, {}, {}]", i, pos[0], pos[2], pos[1]);

			auto* waypoint = pkt.add_waypoints();
			waypoint->set_x(pos[0]);
			waypoint->set_y(pos[2]);
			waypoint->set_z(pos[1]);

			serverWaypoints.emplace_back(*waypoint);
		}
		playerRef->SetMoveInfo(serverWaypoints, GetTickCount64(), 500.0f);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		BroadCast(sendBuffer);
	});
}

void FieldInstance::LeavePlayer(shared_ptr<PlayerCharacter> player, shared_ptr<FieldInstance> nextField)
{
	DoAsync([this, player = std::move(player), nextField = std::move(nextField)]() {
		_players.erase(player);
		player->SetField(nullptr);

		// TODO : 디스폰 패킷 전송

		if(!nextField) return;
		nextField->EnterPlayer(player);
	});
}

void FieldInstance::FindPath(const dtReal* startPos, const dtReal* endPos, OUT dtQueryResult& pathResult)
{
	auto start = std::chrono::high_resolution_clock::now();

	dtQueryFilter Filter;

	dtReal Extents[3] = { 10.0, 100.0, 10.0 };

	dtPolyRef StartPolyRef = 0;
	dtPolyRef EndPolyRef = 0;
	dtReal StartNearestPt[3];
	dtReal EndNearestPt[3];

	// (A) 시작점 근처의 폴리곤 찾기
	_navQuery->findNearestPoly(startPos, Extents, &Filter, &StartPolyRef, StartNearestPt);

	// (B) 도착점 근처의 폴리곤 찾기
	_navQuery->findNearestPoly(endPos, Extents, &Filter, &EndPolyRef, EndNearestPt);

	if (!StartPolyRef || !EndPolyRef)
	{
		LOG_WARN("[Path] Failed to find start or end polygon on NavMesh!");
	}

	dtQueryResult result;
	int PathCount = 0;
	auto costLimit = DBL_MAX;
	dtReal totalCost;

	_navQuery->findPath(StartPolyRef, EndPolyRef, StartNearestPt, EndNearestPt, costLimit, &Filter, result, &totalCost);

	// (D) 실제 이동 좌표 구하기 (String Pulling)
	// 폴리곤 ID만으로는 이동을 못하니까, 실제 꺾이는 지점(Waypoints) 좌표를 뽑아야 함
	static constexpr int MAX_SMOOTH = 256;
	unsigned char StraightPathFlags[MAX_SMOOTH];
	dtPolyRef StraightPathRefs[MAX_SMOOTH];
	result.copyRefs(StraightPathRefs, result.size());


	_navQuery->findStraightPath(StartNearestPt, EndNearestPt, StraightPathRefs, result.size(), pathResult);
	if(pathResult.size() > 0)
	{
		LOG_DEBUG("[Path] Found Straight Path!");
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	LOG_DEBUG("[Path] Execution Time : {}us", duration);
}
