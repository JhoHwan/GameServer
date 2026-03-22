#pragma once
#include <condition_variable>
#include <functional>

#include "Singleton.h"
#include "Types.h"

class PreparedStatement;
class DbConnection;
using DbJob = std::function<void(const DbConnection&, const std::vector<PreparedStatement>&)>;

class DbThreadPool : public Singleton<DbThreadPool>
{
public:
    DbThreadPool(int32 threadCount, std::string  connectionString);
    ~DbThreadPool() = default;

    void Init();
    void PushJob(DbJob job);

private:
    void WorkerMain(std::stop_token token);

private:
    USE_LOCK

    int32 _threadCount;
    std::string connectionString;
    std::vector<std::jthread> _threads;
    std::queue<DbJob> _jobQueue;
    std::string _connectionString;
    std::condition_variable_any _cv;
};
