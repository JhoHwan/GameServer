#pragma once

class PlayerCharacter;

class GameSession : public PacketSession
{
public:
    GameSession();
    ~GameSession() override;
protected:
    void OnRecvPacket(BYTE* buffer, int32 len) override;
    void OnConnected() override;
    void OnDisconnected() override;

public:
    void SetTimeOut(uint64 time, const string& log);
    void CancelTimeOut();
    void SendPacket(const SendBufferRef& sendBuffer);

    void SetPlayer(const shared_ptr<PlayerCharacter>& player) { _playerRef = player; }
    shared_ptr<PlayerCharacter> GetPlayer() const { return _playerRef; }
    shared_ptr<JobQueue> GetJobQueue() { return _jobQueue; }

    void SendPing();
    void HandlePong(const uint64& T1, const uint64& T2, const uint64& T3);
    int64 GetOffset() const { return _serverOffset.load(); }

private:
    using Session::Send;

private:
    shared_ptr<PlayerCharacter> _playerRef;
    shared_ptr<JobQueue> _jobQueue;
    atomic<uint64> _timeOutToken;

    constexpr static int32 INITIAL_PING_COUNT = 5;
    constexpr static int32 INITIAL_PING_INTERVAL = 400;
    constexpr static int32 DEFAULT_PING_INTERVAL = 5000;

    atomic<uint64> _lastPongTime{0};
    atomic<bool> _isInitialSync {true};
    atomic<int32> _syncPingsReceived{0};
    atomic<int64> _accumulatedOffset{0};

    atomic<int64> _serverOffset{0};
    atomic<uint32> _prevLatency {0};
};

