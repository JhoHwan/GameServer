#pragma once
#include "Session.h"

class PlayerCharacter;

class GameSession : public PacketSession
{
public:
    GameSession();
    virtual ~GameSession() override {}
protected:
    void OnRecvPacket(BYTE* buffer, int32 len) override;
    void OnConnected() override;

public:
    void SetTimeOut(uint64 time, const string& log);
    void CancelTimeOut();

    void SetPlayer(const weak_ptr<PlayerCharacter>& player) { _playerRef = player; }
    shared_ptr<PlayerCharacter> GetPlayer() const { return _playerRef.lock(); }
    shared_ptr<JobQueue> GetJobQueue() { return _jobQueue; }
private:
    weak_ptr<PlayerCharacter> _playerRef;
    shared_ptr<JobQueue> _jobQueue;
    atomic<uint32> _timeOutToken;
};

