#pragma once
#include "Session.h"
#include "JobQueue.h"
#include <atomic>

class DummySession : public PacketSession, public AsyncActor
{
public:
	DummySession() = default;
	virtual void OnConnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnDisconnected() override;

    void SetEnteredField(bool value) { _isEnteredField = value; }
    bool IsEnteredField() const { return _isEnteredField; }

    void SendRandomMove();
    void SendTimeSync();

private:
    std::atomic<bool> _isEnteredField = false;
};

extern std::atomic<int32> GConnectedCount;
extern std::atomic<bool> GIsRunning;
