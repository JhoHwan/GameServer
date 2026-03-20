#include "pch.h"
#include "Field.h"

#include <filesystem>
#include <utility>

#include "FieldManager.h"
#include "Contents/Player.h"
#include "GameSession.h"
#include "LogManager.h"
#include "Contents/GameManager.h"
#include "Packet/ServerPacketHandler.h"
#include "Detour/Include/DetourNavMeshQuery.h"
#include "Util/Time.h"


Field::Field(uint64 id, const FieldData* fieldData) : _navMesh(fieldData->NavMesh), _id(id), _fieldData(fieldData)
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
	LOG_DEBUG(Default, "FieldInstance : {}{} is Destroyed", GetMapID(), GetInstanceID());
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

void Field::BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except)
{
	DoAsync([self = shared_from_this(), sendBuffer = std::move(sendBuffer), except]()
	{
		vector<weak_ptr<GameSession>> sessions;
		sessions.reserve(self->_players.size());

		for (auto& player : self->_players)
		{
			if (except == player) continue;
			auto session = player->GetSession();
			if (session) sessions.push_back(session);
		}

		for (const weak_ptr<GameSession>& sessionRef : sessions)
		{
			if(shared_ptr<GameSession> session = sessionRef.lock())
				session->SendPacket(sendBuffer);
		}
	});
}

void Field::LeavePlayer(const shared_ptr<PlayerCharacter>& player)
{
	if(_players.contains(player))
	{
		_players.erase(player);
	}

	if(_currentPlayerCount.fetch_sub(1) == 1)
	{
		JobRef job = make_shared<Job>([self = shared_from_this(), fieldId =_id, token = _destroyToken.load()]()
		{
			if(self->_destroyToken.load() != token) return;
			GFieldManager.Destroy(fieldId);
		});
		LJobTimer.Reserve(10000, GetJobQueue(), job);
	}
}

void Field::UpdatePlayerPosition()
{
	//LOG_DEBUG(FieldInstance, "UpdatePlayerPosition");

	uint64 now = Time::GetServerTime();
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

void Field::Despawn(uint64 id)
{
	DoAsync([self = shared_from_this(), id]()
	{
		if(!self->_objects.contains(id)) return;

		auto& object = self->_objects[id];
		object->OnDespawn();
		self->_objects.erase(id);
	});
}

void Field::HandleRequestUsePortal(const weak_ptr<PlayerCharacter>& playerRef, uint32 portalId)
{
	DoAsync([self = shared_from_this(), playerRef = playerRef, portalId]()
	{
		shared_ptr<PlayerCharacter> player = playerRef.lock();
		if(player == nullptr) return;
		if(self->_fieldData->FieldsPortals.size() <= portalId) return;

		auto portalData = self->_fieldData->FieldsPortals[portalId];
		Vector3 playerPos = player->GetCurrentPosition(Time::GetServerTime());
		auto targetPortalId = portalData.TargetPortalId;
		auto targetMapId = portalData.TargetMapId;
		if(500.0f <= Vector3::Dist2D(portalData.Position, playerPos))
		{
			return;
		}
		LOG_DEBUG(Field, "[Player {}] Request Use Portal. Portal ID : {}, Target MapId: {}", player->GetInstanceID(), portalId, targetMapId);

		GameManager::Instance().ProcessMoveField(player, targetMapId, targetPortalId);
	});
}

void Field::HandleRequestMove(const weak_ptr<PlayerCharacter>& playerRef, const Vector3& dest, const uint64& startServerTick)
{
	DoAsync([self = shared_from_this(), playerRef = playerRef, dest = dest, startServerTick = startServerTick]()
	{
		auto player = playerRef.lock();
		if(player == nullptr || !self->_players.contains(player)) return;

		vector<Vector3> wayPoints;

		self->FindPath(player->GetCurrentPosition(startServerTick), dest, wayPoints);

		if(wayPoints.empty()) return;

		auto speed = player->GetMoveSpeed();

		vector<uint64> moveArrivalTimes;
		moveArrivalTimes.reserve(wayPoints.size());
		moveArrivalTimes.push_back(startServerTick);

		uint64 totalTime = startServerTick;
		float totalDist = 0;
		for(int i = 1; i < wayPoints.size(); i++)
		{
			float dist = Vector3::Dist(wayPoints[i-1], wayPoints[i]);
			totalDist += dist;

			float seconds = dist / speed;
			auto timeToTravel = static_cast<uint64>(seconds * 1000.0f);
			totalTime += timeToTravel;
			moveArrivalTimes.push_back(totalTime);
		}

		Protocol::SC_MOVE_PATH pkt;
		pkt.set_object_id(player->GetId());
		pkt.set_start_server_tick(startServerTick);
		for (int i = 0; i < wayPoints.size(); i++)
		{
			Protocol::WayPoint* wayPoint = pkt.add_waypoints();
			wayPoint->mutable_pos()->CopyFrom(wayPoints[i].ToProto());
			wayPoint->set_arrival_offset_ms(static_cast<uint32>(moveArrivalTimes[i] - startServerTick));
		}

		self->BroadCast(ServerPacketHandler::MakeSendBuffer(pkt));

		player->SetMoveInfo(std::move(wayPoints), std::move(moveArrivalTimes), startServerTick);
	});
}

void Field::EnterPlayer(const shared_ptr<PlayerCharacter>& player)
{
	_destroyToken.fetch_add(1);
	_players.insert(player);
}


void Field::FindPath(const Vector3& startPos, const Vector3& endPos, OUT std::vector<Vector3>& outWayPoints)
{
	auto start = std::chrono::high_resolution_clock::now();

	dtQueryFilter filter;
	filter.setIncludeFlags(0xffff);
	filter.setExcludeFlags(0);

	float extents[3] = { 0.5f, 1.0f, 0.5f };

	dtPolyRef startPolyRef = 0;
	dtPolyRef endPolyRef = 0;
	
	// Convert Engine coordinates (cm) to Detour coordinates (m)
	float startPt[3] { (float)startPos.x / 100.0f, (float)startPos.z / 100.0f, (float)startPos.y / 100.0f };
	float endPt[3] = { (float)endPos.x / 100.0f, (float)endPos.z / 100.0f, (float)endPos.y / 100.0f };

	float StartNearestPt[3];
	float EndNearestPt[3];

	_navQuery->findNearestPoly(startPt, extents, &filter, &startPolyRef, StartNearestPt);
	_navQuery->findNearestPoly(endPt, extents, &filter, &endPolyRef, EndNearestPt);

	if (!startPolyRef || !endPolyRef)
	{
		//LOG_DEBUG(PathFind, "Failed to find start or end polygon on NavMesh!");
		return;
	}

	float t = 0;
	float hitNormal[3];
	dtPolyRef rayPath[20];
	int rayPathCount = 0;

	_navQuery->raycast(startPolyRef, startPt, endPt, &filter, &t, hitNormal, rayPath, &rayPathCount, 20);
	if(t >= 1.0)
	{
		//LOG_DEBUG(NavMesh, "Straight Path")
		outWayPoints.push_back(startPos);
		outWayPoints.push_back(endPos);

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		//LOG_DEBUG(PathFind, "Execution Time : {}us", duration);

		return;
	}

	static constexpr int MAX_PATH_POLYS = 2048;
	dtPolyRef path[MAX_PATH_POLYS];
	int pathCount = 0;

	_navQuery->findPath(startPolyRef, endPolyRef, StartNearestPt, EndNearestPt, &filter, path, &pathCount, MAX_PATH_POLYS);

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
		//LOG_DEBUG(PathFind, "Found Straight Path! Points: {}", straightPathCount);
		for (int i = 0; i < straightPathCount; ++i)
		{
			// Detour: X, Y, Z (m) -> Engine: X, Z, Y (cm)
			outWayPoints.emplace_back(straightPath[i*3] * 100.0f, straightPath[i*3+2] * 100.0f, straightPath[i*3+1] * 100.0f);
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	//LOG_DEBUG(PathFind, "Execution Time : {}us", duration);
}
