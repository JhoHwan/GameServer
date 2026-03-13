#include "GameManager.h"

#include "Field/Field.h"
#include "GameObject.h"
#include "GameSession.h"
#include "Player.h"
#include "Protocol.pb.h"
#include "Types.h"
#include "Field/FieldManager.h"
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

        //auto targetMapId = player->GetLoadingMapId();

        // TODO : 입장 로드 맵 아이디 하드 코딩됨
        auto targetMapId = 1000;
        auto fieldData = GFieldManager.GetFieldData(targetMapId);
        if(!fieldData) return;
        auto playerSpawnPos = fieldData->PlayerStarts[0];

        player->SetLoadingInfo(targetMapId, playerSpawnPos);

        loadPacket.set_target_map_id(targetMapId);
        session->Send(ServerPacketHandler::MakeSendBuffer(loadPacket));

        session->CancelTimeOut();
        session->SetTimeOut(60000, "Map Loading");
    });
}

void GameManager::ProcessMoveField(const shared_ptr<PlayerCharacter>& player, uint16 targetMapId, int32 targetPortalId)
{
    auto session = player->GetSession();
    auto oldField = player->GetField();

    if(!oldField) return;

    const FieldData* targetFieldData = GFieldManager.GetFieldData(targetMapId);
    if(!targetFieldData) return;

    Vector3 playerSpawnPos{};
    if(targetPortalId != -1)
    {
        if(targetFieldData->FieldsPortals.size() <= targetPortalId) return;

        playerSpawnPos = targetFieldData->FieldsPortals[targetPortalId].Position;
    }
    else
    {
        playerSpawnPos = targetFieldData->PlayerStarts[0];
    }

    oldField->LeavePlayer(player);
    player->SetLoadingInfo(targetMapId, playerSpawnPos);

    Protocol::SC_START_FIELD_LOADING loadPacket;

    loadPacket.set_target_map_id(player->GetLoadingMapId());
    session->Send(ServerPacketHandler::MakeSendBuffer(loadPacket));

    session->SetTimeOut(60000, "Map Loading");
}
