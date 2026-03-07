#include "GameManager.h"

#include "Field.h"
#include "GameObject.h"
#include "GameSession.h"
#include "Player.h"
#include "Protocol.pb.h"
#include "Types.h"
#include "Packet/ServerPacketHandler.h"
#include "Util/Vector3.h"

void GameManager::ProcessEnterGame(std::weak_ptr<GameSession> session)
{
    DoAsync([sessionWeak = std::move(session)]()
        {
            auto session = sessionWeak.lock();
            if (!session) return;

            // TODO : 토큰 인증 & DB 요청 후 아래 내용 콜백으로 등록
            Protocol::SC_ENTER_GAME_RESULT packet;
            packet.set_success(true); // 추후 DB 요청 결과 or 인증 결과에 따라 변경
            session->Send(ServerPacketHandler::MakeSendBuffer(packet));

            auto player = GameObject::Create<PlayerCharacter>(session);

            Protocol::SC_START_FIELD_LOADING loadPacket;
            loadPacket.set_target_map_id(0);
            loadPacket.mutable_start_pos()->CopyFrom(Vector3::Zero().ToProto());
            session->Send(ServerPacketHandler::MakeSendBuffer(packet));
            session->SetTimeOut(60000, "Map Loading");
        });
}

void GameManager::ProcessMoveField(const std::shared_ptr<PlayerCharacter>& player, uint32 targetMapId)
{
    auto oldField = player->GetField();
    auto targetField = GFieldManager.GetField(targetMapId);

    if(!oldField) return;
    oldField->LeavePlayer(player, targetField);
}
