#pragma once

class PlayerCharacter;

class GameSession : public PacketSession
{
public:
    GameSession();
    ~GameSession() override = default;
protected:
    void OnRecvPacket(BYTE* buffer, int32 len) override;
    void OnConnected() override;
    void OnDisconnected() override;

public:
    void SetTimeOut(uint64 time, const string& log);
    void CancelTimeOut();

    void SetPlayer(const shared_ptr<PlayerCharacter>& player) { _playerRef = player; }
    shared_ptr<PlayerCharacter> GetPlayer() const { return _playerRef; }
    shared_ptr<JobQueue> GetJobQueue() { return _jobQueue; }
private:
    shared_ptr<PlayerCharacter> _playerRef;
    shared_ptr<JobQueue> _jobQueue;
    atomic<uint32> _timeOutToken;
};

