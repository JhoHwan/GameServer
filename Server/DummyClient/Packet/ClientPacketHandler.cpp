#include "pch.h"
#include "ClientPacketHandler.h"
#include "DummySession.h"
#include <iostream>

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_SC_ENTER_GAME_RESULT(SessionRef& session, Protocol::SC_ENTER_GAME_RESULT& pkt)
{
	return true;
}

bool Handle_SC_MOVE_FIELD_FAIL(SessionRef& session, Protocol::SC_MOVE_FIELD_FAIL& pkt)
{
	return false;
}

bool Handle_SC_START_FIELD_LOADING(SessionRef& session, Protocol::SC_START_FIELD_LOADING& pkt)
{
    Protocol::CS_FIELD_LOADING_COMPLETE sendPkt;
    SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(sendPkt);
    session->Send(sendBuffer);
	return true;
}

bool Handle_SC_ENTER_FIELD(SessionRef& session, Protocol::SC_ENTER_FIELD& pkt)
{
    auto dummySession = std::static_pointer_cast<DummySession>(session);
    if (!dummySession->IsEnteredField())
    {
        dummySession->SetEnteredField(true);
        
        // JobQueue를 통한 비동기 루프 시작
        dummySession->DoAsync([dummySession](){dummySession->SendRandomMove();});
        dummySession->DoAsync([dummySession](){dummySession->SendTimeSync();});
    }
	return true;
}


bool Handle_SC_SPAWN_PLAYER(SessionRef& session, Protocol::SC_SPAWN_PLAYER& pkt)
{
	return true;
}

bool Handle_SC_DESPAWN_PLAYER(SessionRef& session, Protocol::SC_DESPAWN_PLAYER& pkt)
{
	return true;
}

bool Handle_SC_MOVE_PATH(SessionRef& session, Protocol::SC_MOVE_PATH& pkt)
{
	return true;
}

bool Handle_SC_TIME_SYNC(SessionRef& session, Protocol::SC_TIME_SYNC& pkt)
{
	return true;
}
