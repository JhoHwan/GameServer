#include "pch.h"
#include "Field.h"
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
		_players.push_back(player);

		player->SetField(GetFieldRef());

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
