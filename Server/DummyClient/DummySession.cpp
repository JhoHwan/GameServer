#include "pch.h"
#include "DummySession.h"
#include "Packet/ClientPacketHandler.h"
#include <random>
#include <thread>
#include <iostream>

void DummySession::OnConnected()
{
    GConnectedCount.fetch_add(1);

	Protocol::CS_REQ_ENTER_GAME Pkt;
    Send(ClientPacketHandler::MakeSendBuffer(Pkt));
}

void DummySession::OnRecvPacket(BYTE* buffer, int32 len)
{
    auto session = std::static_pointer_cast<Session>(GetSessionRef());
    ClientPacketHandler::HandlePacket(session, buffer, len);
}

void DummySession::OnDisconnected()
{
    GConnectedCount.fetch_sub(1);
    _isEnteredField = false;
}

void DummySession::SendRandomMove()
{
    if (!GIsRunning || !_isEnteredField) return;

    static thread_local std::mt19937 gen(std::random_device{}());
    static std::uniform_real_distribution<float> dist(-2000.0f, 2000.0f);

    Protocol::CS_REQUEST_MOVE pkt;
    auto* pos = pkt.mutable_pos();
    pos->set_x(dist(gen));
    pos->set_y(dist(gen));
    pos->set_z(200.0f);

    SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
    Send(sendBuffer);

    // 1~2초 후 본인의 JobQueue에 다시 예약
    static std::uniform_int_distribution<int> intervalDist(1000, 2000);
    JobRef job = make_shared<Job>([self = static_pointer_cast<DummySession>(shared_from_this())]()
    {
        self->SendRandomMove();
    });

    LJobTimer.Reserve(intervalDist(gen), GetJobQueue(), job);
}

void DummySession::SendTimeSync()
{
    if (!GIsRunning || !_isEnteredField) return;

    Protocol::CS_TIME_SYNC pkt;
    pkt.set_client_tick(GetTickCount64());

    SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
    Send(sendBuffer);

    JobRef job = make_shared<Job>([self = static_pointer_cast<DummySession>(shared_from_this())]()
    {
        self->SendTimeSync();
    });

    // 10초 후 본인의 JobQueue에 다시 예약
    LJobTimer.Reserve(10000, GetJobQueue(), job);
}
