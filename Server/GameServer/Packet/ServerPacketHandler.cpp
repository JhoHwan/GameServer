#include "ServerPacketHandler.h"
#include "Contents/Player.h"
#include "../Contents/Field/Field.h"
#include "GameSession.h"
#include "LogManager.h"
#include "Contents/GameManager.h"
#include "Contents/Field/FieldManager.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len)
{
    return false;
}

bool Handle_CS_REQ_ENTER_GAME(SessionRef& session, Protocol::CS_REQ_ENTER_GAME& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    gSession->CancelTimeOut();

    GameManager::Instance().ProcessEnterGame(gSession);
    return true;
}

bool Handle_CS_REQ_MOVE_FIELD(SessionRef& session, Protocol::CS_REQ_MOVE_FIELD& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    GameManager::Instance().ProcessMoveField(gSession->GetPlayer(), pkt.map_id());
    return true;
}

bool Handle_CS_FIELD_LOADING_COMPLETE(SessionRef& session, Protocol::CS_FIELD_LOADING_COMPLETE& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    gSession->CancelTimeOut();
    gSession->SetTimeOut(20000, "HeartBeat");

    shared_ptr<PlayerCharacter> player = gSession->GetPlayer();
    if (!player)
    {
        gSession->Disconnect("Invalid Error");
        return false;
    }

    // TODO : 하드 코딩 됨
    auto field = GFieldManager.GetField(0);
    if(field) field->EnterPlayer(player);

    return true;
}

bool Handle_CS_REQUEST_MOVE(SessionRef& session, Protocol::CS_REQUEST_MOVE& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    LOG_INFO("Player{} Request Move : [{}, {}, {}]", 0, pkt.pos().x(), pkt.pos().y(), pkt.pos().z());

    gSession->GetPlayer()->GetField()->PlayerRequestMove(gSession->GetPlayer(), pkt.pos());

    return true;
}

bool Handle_CS_TIME_SYNC(SessionRef& session, Protocol::CS_TIME_SYNC& pkt)
{
    LOG_DEBUG("Client Request Time Sync");
    shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);
    gameSession->CancelTimeOut();
    gameSession->SetTimeOut(20000, "HeartBeat");

    Protocol::SC_TIME_SYNC res;
    res.set_client_tick(pkt.client_tick());
    res.set_server_tick(GetTickCount64());

    SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(res);
    session->Send(sendBuffer);

    return true;
}
