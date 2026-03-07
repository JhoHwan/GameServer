#pragma once
#include <fstream>
#include <stop_token>
#include <thread>
#include <format>

#include "concurrentqueue.h"

#define LOG_INFO(Format, ...) LogManager::Instance().WriteLog(ELogLevel::Info, std::format(Format __VA_OPT__(,) __VA_ARGS__));

enum class ELogLevel : uint8
{
    Debug = 0,
    Info,
    Warning,
    Error,
    Fatal,
};

struct LogData
{
    ELogLevel level;
    uint32 threadId;
    std::chrono::system_clock::time_point timestamp;
    std::string message;
};

class LogManager : public Singleton<LogManager>
{
public:
    LogManager() = default;
    ~LogManager() { Stop(); }

    bool Init(ELogLevel logLevel);
    void ChangeLogLevel(const ELogLevel logLevel) {_logLevel = logLevel;}
    void Stop();
    void WriteLog(ELogLevel level, std::string message);


private:
    void ThreadMain(const std::stop_token& stopToken);

    static constexpr std::string_view GetLogText(ELogLevel level)
    {
        switch(level)
        {
            case ELogLevel::Debug:   return "Debug";
            case ELogLevel::Info:     return "Info";
            case ELogLevel::Warning: return "Warning";
            case ELogLevel::Error:   return "Error";
            case ELogLevel::Fatal:   return "Fatal";
            default:                 return "Unknown";
        }
    }

private:
    std::jthread _logThread;
    moodycamel::ConcurrentQueue<LogData> _logQueue;

    std::ofstream _file;

    ELogLevel _logLevel;
};