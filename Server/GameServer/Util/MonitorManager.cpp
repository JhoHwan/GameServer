#include "MonitorManager.h"

#include "LogManager.h"



void MonitorManager::Init(std::chrono::seconds interval)
{
    _logInterval = interval;
    _monitorThread = std::jthread([this](const std::stop_token& st) {
        this->ThreadMain(st);
    });
}


void MonitorManager::ThreadMain(const std::stop_token& st)
{
    while(!st.stop_requested())
    {
        this_thread::sleep_for(_logInterval);
        LOG_INFO(Monitor, "[CCU]: {} ", GetCCU());
        UpdatePPS();
        LOG_INFO(Monitor, "[InPPS]: {}", GetInPacket() / _logInterval.count());
        LOG_INFO(Monitor, "[OutPPS]: {}", GetOutPacket() / _logInterval.count());

        LOG_INFO(Monitor, "[Field]: {}", GetFieldCount());
    }
}
