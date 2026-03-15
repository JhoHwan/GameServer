#pragma once

#include "Singleton.h"

class MonitorManager : public Singleton<MonitorManager>
{
public:
    void Init(std::chrono::seconds interval);
    ~MonitorManager() = default;

    void AddCCU() { _ccu.fetch_add(1); }
    void ReleaseCCU() { _ccu.fetch_sub(1); }
    [[nodiscard]] uint32 GetCCU() const { return _ccu.load(); }

    void AddInPacket() { _totalInPackets.fetch_add(1); }
    void AddOutPacket() { _totalOutPackets.fetch_add(1); }

    [[nodiscard]] uint32 GetInPacket() const { return _inPPS; }
    [[nodiscard]] uint32 GetOutPacket() const { return _outPPS; }

    void AddFieldCount() { _fieldCount.fetch_add(1); }
    void ReleaseFieldCount() { _fieldCount.fetch_sub(1); }
    [[nodiscard]] uint32 GetFieldCount() const { return _fieldCount.load(); }

private:
    void UpdatePPS()
    {
        uint64 currentIn = _totalInPackets.load();
        uint64 currentOut = _totalOutPackets.load();

        _inPPS = currentIn - _lastInPackets;
        _outPPS = currentOut - _lastOutPackets;

        _lastInPackets = currentIn;
        _lastOutPackets = currentOut;
    }

    void ThreadMain(const stop_token& st);

private:
    std::chrono::seconds _logInterval = 1s;
    std::jthread _monitorThread;
    std::atomic<uint32> _ccu;
    std::atomic<uint64> _totalInPackets;
    std::atomic<uint64> _totalOutPackets;

    uint64 _lastInPackets = 0;
    uint64 _lastOutPackets = 0;

    uint64 _inPPS = 0;
    uint64 _outPPS = 0;

    std::atomic<uint32> _fieldCount;
};

inline MonitorManager& GMonitorManager = MonitorManager::Instance();
