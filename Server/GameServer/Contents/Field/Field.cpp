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


Field::Field(uint16 id, dtNavMesh* navMesh) : _id(id), _navMesh(navMesh)
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
	LOG_DEBUG("FieldInstance : {} is Destroyed", _id);
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

void Field::EnterPlayer(shared_ptr<PlayerCharacter> player)
{
	DoAsync([this, player = std::move(player)]()
	{
		_players.insert(player);
		player->Transform()->SetPos({6168.718262,-181.932266,150});
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

void Field::BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except)
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

void Field::PlayerRequestMove(weak_ptr<PlayerCharacter> player, const Protocol::Vector3& dest)
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

		Vector3 startPos = playerRef->GetCurrentPosition(GetTickCount64());
		std::swap(startPos.y, startPos.z);
		Vector3 endPos(dest);
		std::swap(endPos.y, endPos.z);

		std::vector<Vector3> serverWaypoints;
		FindPath(startPos, endPos, serverWaypoints);

		Protocol::SC_MOVE_PATH pkt;
		pkt.set_start_server_tick(GetTickCount64());
		pkt.set_object_id(playerRef->GetId());

		for (int i = 0; i < serverWaypoints.size(); i++)
		{
			LOG_DEBUG("[{}] : [{}, {}, {}]", i, serverWaypoints[i].x, serverWaypoints[i].y, serverWaypoints[i].z);

			auto* waypoint = pkt.add_waypoints();
			waypoint->set_x(serverWaypoints[i].x);
			waypoint->set_y(serverWaypoints[i].y);
			waypoint->set_z(serverWaypoints[i].z);

			serverWaypoints[i] = Vector3(*waypoint); // Ensure serverWaypoints matches exact packet format if needed, but they are already identical.
		}
		playerRef->SetMoveInfo(std::move(serverWaypoints), GetTickCount64(), 500.0f);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		BroadCast(sendBuffer);
	});
}

void Field::LeavePlayer(shared_ptr<PlayerCharacter> player, shared_ptr<Field> nextField)
{
	DoAsync([this, player = std::move(player), nextField = std::move(nextField)]() {
		player->SetField(nullptr);
		_players.erase(player);

		// TODO : 디스폰 패킷 전송

		if(_players.empty())
		{
			JobRef job = make_shared<Job>([fieldId = _id]() {
				GFieldManager.Destroy(fieldId);
			});
			LJobTimer.Reserve(10000, GetJobQueue(), job);
		}

		if(!nextField) return;
		nextField->EnterPlayer(player);
	});
}

void Field::UpdatePlayerPosition()
{
	//LOG_DEBUG("[FieldInstance] UpdatePlayerPosition");

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
	float startPt[3] { (float)startPos.x / 100.0f, (float)startPos.y / 100.0f, (float)startPos.z / 100.0f };
	float endPt[3] = { (float)endPos.x / 100.0f, (float)endPos.y / 100.0f, (float)endPos.z / 100.0f };

	float StartNearestPt[3];
	float EndNearestPt[3];

	// (A) 시작점 근처의 폴리곤 찾기
	_navQuery->findNearestPoly(startPt, extents, &filter, &StartPolyRef, StartNearestPt);

	// (B) 도착점 근처의 폴리곤 찾기
	_navQuery->findNearestPoly(endPt, extents, &filter, &EndPolyRef, EndNearestPt);

	if (!StartPolyRef || !EndPolyRef)
	{
		LOG_WARN("[Path] Failed to find start or end polygon on NavMesh!");
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
		LOG_DEBUG("[Path] Found Straight Path! Points: {}", straightPathCount);
		for (int i = 0; i < straightPathCount; ++i)
		{
			// Detour: X, Y, Z (m) -> Engine: X, Z, Y (cm)
			pathResult.push_back(Vector3(straightPath[i*3] * 100.0f, straightPath[i*3+2] * 100.0f, straightPath[i*3+1] * 100.0f));
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	LOG_DEBUG("[Path] Execution Time : {}us", duration);
}
