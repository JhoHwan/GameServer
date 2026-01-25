#include "pch.h"
#include "Field.h"
#include "Contents\Player.h"
#include "GameSession.h"
#include "Packet\ServerPacketHandler.h"

FieldManager& GFieldManager = FieldManager::Instance();

void Field::EnterPlayer(shared_ptr<PlayerCharacter> player)
{
	{
		WRITE_LOCK;
		_players.push_back(player);
	}

	player->SetField(shared_from_this());

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
			READ_LOCK;
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
}

void Field::BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except)
{
	vector<SessionRef> sessions;
	{
		READ_LOCK;
		sessions.reserve(_players.size());
		for (auto player : _players)
		{
			if (except == player) continue;
			auto session = player->GetSession();
			if (session) sessions.push_back(session);
		}
	}

	for (auto& session : sessions)
	{
		session->Send(sendBuffer);
	}
}
