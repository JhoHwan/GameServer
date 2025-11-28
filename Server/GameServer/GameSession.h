#pragma once
#include <Session.h>
class GameSession : public PacketSession
{
    void OnRecvPacket(BYTE* buffer, int32 len) override;
};

