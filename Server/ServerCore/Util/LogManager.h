#pragma once
#include <fstream>

enum class ELogLevel : uint8
{
    Debug,
    Log,
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
    ~LogManager() = default;

    void Init();
    void Stop();
    void WriteLog(ELogLevel level, std::string message);

private:
    void ThreadMain(const std::stop_token& stopToken);

    static constexpr std::string_view GetLogText(ELogLevel level)
    {
        switch(level)
        {
            case ELogLevel::Debug:   return "Debug";
            case ELogLevel::Log:     return "Log";
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
};