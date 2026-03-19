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

    GameManager::Instance().ProcessEnterGame(gSession);
    return true;
}

bool Handle_CS_REQ_MOVE_FIELD(SessionRef& session, Protocol::CS_REQ_MOVE_FIELD& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    GameManager::Instance().ProcessMoveField(gSession->GetPlayer(), pkt.map_id());
    return true;
}

bool Handle_CS_USE_PORTAL(SessionRef& session, Protocol::CS_USE_PORTAL& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    auto player = gSession->GetPlayer();
    if (player == nullptr) return true;

    auto field = player->GetField();
    if(field == nullptr) return true;

    field->HandleRequestUsePortal(player, pkt.portal_id());

    return true;
}

bool Handle_CS_FIELD_LOADING_COMPLETE(SessionRef& session, Protocol::CS_FIELD_LOADING_COMPLETE& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);

    shared_ptr<PlayerCharacter> player = gSession->GetPlayer();
    if (!player)
    {
        gSession->Disconnect("Invalid Error");
        return false;
    }

    auto field = GFieldManager.GetField(player->GetLoadingMapId());
    if(field) field->EnterPlayer(player);

    gSession->CancelTimeOut();

    return true;
}

bool Handle_CS_REQUEST_MOVE(SessionRef& session, Protocol::CS_REQUEST_MOVE& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    auto player = gSession->GetPlayer();
    if (player == nullptr) return false;

    //LOG_INFO(PathFind, "Player{} Request Move : [{}, {}, {}]", player->GetId(), pkt.pos().x(), pkt.pos().y(), pkt.pos().z());

    auto startServerTick = pkt.client_tick() + gSession->GetOffset();
    player->HandleMoveRequest(pkt.pos(), startServerTick);

    return true;
}

bool Handle_CS_PONG(SessionRef& session, Protocol::CS_PONG& pkt)
{
    auto gSession = static_pointer_cast<GameSession>(session);
    gSession->HandlePong(pkt.server_send_tick(), pkt.client_recv_tick(), pkt.client_send_tick());
    return true;
}


