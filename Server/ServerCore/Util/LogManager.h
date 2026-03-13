#pragma once
#include <fstream>
#include <stop_token>
#include <thread>
#include <format>

#include "concurrentqueue.h"

#define LOG_INFO(Category, Format, ...) LogManager::Instance().WriteLog(ELogLevel::Info, #Category, std::format(Format __VA_OPT__(,) __VA_ARGS__));
#define LOG_DEBUG(Category, Format, ...) LogManager::Instance().WriteLog(ELogLevel::Debug, #Category, std::format(Format __VA_OPT__(,) __VA_ARGS__));
#define LOG_WARN(Category, Format, ...) LogManager::Instance().WriteLog(ELogLevel::Warning, #Category, std::format(Format __VA_OPT__(,) __VA_ARGS__));
#define LOG_ERROR(Category, Format, ...) LogManager::Instance().WriteLog(ELogLevel::Error, #Category, std::format(Format __VA_OPT__(,) __VA_ARGS__));

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
    std::string category;
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
    void WriteLog(ELogLevel level, std::string category, std::string message);


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