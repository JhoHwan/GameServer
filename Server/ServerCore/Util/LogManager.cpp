#include "LogManager.h"

#include <filesystem>
#include <fstream>
#include <utility>
#include <format>

void LogManager::Init()
{
    std::filesystem::path baseDir = std::filesystem::current_path() / "Log";
    if (!std::filesystem::exists(baseDir))
    {
        std::filesystem::create_directory(baseDir);
    }

    std::filesystem::path logPath = baseDir / "Log.txt";
    _file.open(logPath, std::ios::app | std::ios::out);
    if(!_file.is_open())
    {
        cout << "LogManager::Init() Error" << endl;
    }

    _logThread = std::jthread([this](std::stop_token st) {
        this->ThreadMain(std::move(st));
    });

    _file << "LOG" << endl;
    cout << "LogManager::Init() Success" << "\n";
}

void LogManager::Stop()
{
    if (_logThread.joinable())
    {
        _logThread.request_stop();
        _logThread.join();
    }

    if(_file.is_open())
    {
        _file.flush();
        _file.close();
    }
}

void LogManager::WriteLog(ELogLevel level, std::string message)
{
    LogData data;
    data.level = level;
    data.message = std::move(message);
    data.timestamp = std::chrono::system_clock::now();
    data.threadId = LThreadId;

    _logQueue.enqueue(std::move(data));
}

void LogManager::ThreadMain(const std::stop_token& stopToken)
{
    while(!stopToken.stop_requested() || _logQueue.size_approx() > 0)
    {
        LogData datas[32];
        auto outCount = _logQueue.try_dequeue_bulk(datas, 32);
        if(outCount == 0)
        {
            if(stopToken.stop_requested()) break;

            std::this_thread::sleep_for(10ms);
            continue;
        }

        for(int i = 0; i < outCount; i++)
        {
            auto timeT = std::chrono::system_clock::to_time_t(datas[i].timestamp);
            std::tm timeInfo{};

#ifdef _WIN32
            localtime_s(&timeInfo, &timeT);
#else
            localtime_r(&timeT, &timeInfo);
#endif

            char timeBuf[64];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &timeInfo);

            std::string logStr = std::format("[{}] [{}] [Thread: {}] {}\n",
                GetLogText(datas[i].level),
                timeBuf,
                datas[i].threadId,
                datas[i].message);

            _file << logStr;
            std::cout << logStr;
        }

        _file.flush();
    }
}
