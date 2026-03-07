#include "pch.h"
#include "ServerPacketHandler.h"
#include "Contents/Player.h"
#include "Contents/Field.h"
#include "GameSession.h"
#include "LogManager.h"
#include "Contents/GameManager.h"

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

    shared_ptr<PlayerCharacter> player = gSession->GetPlayer();
    if (!player)
    {
        gSession->Disconnect("Invalid Error");
        return false;
    }

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


bool Handle_CS_PING(SessionRef& session, Protocol::CS_PING& pkt)
{
    Protocol::SC_PONG packet;
    packet.set_id(pkt.id());
    auto buffer = ServerPacketHandler::MakeSendBuffer(packet);
    if (!buffer) return false;
    session->Send(buffer);

    LOG_INFO("Client Ping {}", packet.id());

    return true;
}
