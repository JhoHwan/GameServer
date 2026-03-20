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

void GameManager::ProcessEnterGame(const std::weak_ptr<GameSession>& sessionRef)
{
    auto session = sessionRef.lock();
    if (!session) return;

    // TODO : 토큰 인증 & DB 요청 후 아래 내용 콜백으로 등록
    Protocol::SC_ENTER_GAME_RESULT packet;
    packet.set_success(true); // 추후 DB 요청 결과 or 인증 결과에 따라 변경
    session->SendPacket(ServerPacketHandler::MakeSendBuffer(packet));

    // TODO : 입장 로드 맵 아이디 하드 코딩됨(DB연동 후 받아와야함)
    auto targetMapId = 1000;
    auto fieldData = GFieldManager.GetFieldData(targetMapId);
    if(!fieldData) return;
    Vector3 playerSpawnPos {fieldData->PlayerStarts[0]};

    session->SetLoadingInfo(targetMapId, playerSpawnPos);

    Protocol::SC_START_FIELD_LOADING loadPacket;
    loadPacket.set_target_map_id(targetMapId);
    session->SendPacket(ServerPacketHandler::MakeSendBuffer(loadPacket));

    session->SetTimeOut(60000, "Map Loading");
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

    oldField->Despawn(player->GetId());
    session->SetLoadingInfo(targetMapId, playerSpawnPos);

    Protocol::SC_START_FIELD_LOADING loadPacket;
    loadPacket.set_target_map_id(targetMapId);
    session->SendPacket(ServerPacketHandler::MakeSendBuffer(loadPacket));

    session->SetTimeOut(60000, "Map Loading");
}
