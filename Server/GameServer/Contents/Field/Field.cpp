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

void Field::EnterPlayer(weak_ptr<PlayerCharacter> player)
{
	_destroyToken.fetch_add(1);

	DoAsync([self = shared_from_this(), playerWeak = std::move(player)]()
	{
		auto player = playerWeak.lock();
		if(!player) return;

		self->_players.insert(player);

		auto playerSpawnPos = player->GetPendingSpawnPos();

		player->Transform()->SetPos(playerSpawnPos);
		player->SetField(self);

		player->SetLoadingInfo(0, Vector3::Zero());

		{
			Protocol::SC_ENTER_FIELD packet;
			player->GetObjectInfo(packet.mutable_my_info()->mutable_object_info());
			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
			auto session = player->GetSession();
			if (session) session->SendPacket(sendBuffer);
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
			if(self->_players.size() <= 1) return;

			Protocol::SC_SPAWN_PLAYER packet;
			vector<Protocol::SC_MOVE_PATH> movePackets;
			movePackets.reserve(self->_players.size());
			for (const shared_ptr<PlayerCharacter>& other : self->_players)
			{
				if (other == player) continue;
				other->GetObjectInfo(packet.add_info()->mutable_object_info());
				if(other->IsMoving())
				{
					uint64 now = GetTickCount64();
					Vector3 currentPos = other->GetCurrentPosition(now);

					Protocol::SC_MOVE_PATH movePacket;
					movePacket.set_object_id(other->GetId());
					movePacket.set_start_server_tick(now);

					{
						auto* firstWP = movePacket.add_waypoints();
						firstWP->mutable_pos()->CopyFrom(currentPos.ToProto());
						firstWP->set_arrival_offset_ms(0);
					}

					const auto& waypoints = other->GetWaypoints();
					const auto& arrivalTimes = other->GetArrivalTimes();

					for (size_t i = 0; i < arrivalTimes.size(); ++i)
					{
						if (arrivalTimes[i] <= now) continue;

						auto* wp = movePacket.add_waypoints();
						wp->mutable_pos()->CopyFrom(waypoints[i].ToProto());

						auto offset = static_cast<uint32>(arrivalTimes[i] - now);
						wp->set_arrival_offset_ms(offset);
					}

					if (movePacket.waypoints_size() > 0)
					{
						movePackets.push_back(std::move(movePacket));
					}
				}
			}

			if (packet.info_size() == 0) return;
			auto session = player->GetSession();
			if (!session) return;

			{
				SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
				session->SendPacket(sendBuffer);
			}

			for(auto& movePacket : movePackets)
			{
				SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePacket);
				session->SendPacket(sendBuffer);
			}

		}
	});
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

void Field::LeavePlayer(shared_ptr<PlayerCharacter> player)
{
	DoAsync([self = shared_from_this(), player = std::move(player)]() {
		player->SetField(nullptr);

		Protocol::SC_DESPAWN_PLAYER pkt;
		pkt.set_player_id(player->GetId());
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		self->BroadCast(sendBuffer);

		if(self->_players.contains(player))
		{
			self->_players.erase(player);
		}

		if(self->_currentPlayerCount.fetch_sub(1) == 1)
		{
			JobRef job = make_shared<Job>([self, fieldId = self->_id, Token = self->_destroyToken.load()]()
			{
				if(self->_destroyToken.load() != Token) return;
				GFieldManager.Destroy(fieldId);
			});
			LJobTimer.Reserve(10000, self->GetJobQueue(), job);
		}
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

void Field::HandleRequestUsePortal(const weak_ptr<PlayerCharacter>& playerRef, uint32 portalId)
{
	DoAsync([self = shared_from_this(), playerRef = playerRef, portalId]()
	{
		shared_ptr<PlayerCharacter> player = playerRef.lock();
		if(player == nullptr) return;
		if(self->_fieldData->FieldsPortals.size() <= portalId) return;
		//LOG_DEBUG(Field, "[Player {}] Request Use Portal. Portal ID : {}", player->GetInstanceID(), portalId);

		auto portalData = self->_fieldData->FieldsPortals[portalId];
		Vector3 playerPos = player->GetCurrentPosition(GetTickCount64());
		auto targetPortalId = portalData.TargetPortalId;

		if(500.0f <= Vector3::Dist2D(portalData.Position, playerPos))
		{
			return;
		}

		GameManager::Instance().ProcessMoveField(player, portalData.TargetMapId, targetPortalId);
	});
}

void Field::HandleRequestMove(const weak_ptr<PlayerCharacter>& playerRef, const Vector3& dest)
{
	DoAsync([self = shared_from_this(), playerRef = playerRef, dest = dest]()
	{
		auto player = playerRef.lock();
		if(player == nullptr || !self->_players.contains(player)) return;

		auto now = GetTickCount64();
		vector<Vector3> wayPoints;

		self->FindPath(player->GetCurrentPosition(now), dest, wayPoints);

		if(wayPoints.empty()) return;

		auto speed = player->GetMoveSpeed();

		vector<uint64> moveArrivalTimes;
		moveArrivalTimes.reserve(wayPoints.size());
		moveArrivalTimes.push_back(now);

		uint64 totalTime = now;
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
		pkt.set_start_server_tick(now);
		for(int i = 0; i < wayPoints.size(); i++)
		{
			Protocol::WayPoint* wayPoint = pkt.add_waypoints();
			wayPoint->mutable_pos()->CopyFrom(wayPoints[i].ToProto());
			wayPoint->set_arrival_offset_ms(static_cast<uint32>(moveArrivalTimes[i] - now));
		}

		self->BroadCast(ServerPacketHandler::MakeSendBuffer(pkt));

		player->SetMoveInfo(std::move(wayPoints), std::move(moveArrivalTimes), now);
	});
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
