#include "DbThreadPool.h"

#include <utility>

#include "DbConnection.h"
#include "PreparedStatement.h"

DbThreadPool::DbThreadPool(int32 threadCount, std::string connectionString)
    :_threadCount(threadCount), _connectionString(std::move(connectionString))
{
}

void DbThreadPool::Init()
{
    for(int i = 0; i < _threadCount; i++)
    {
        _threads.emplace_back([this](std::stop_token token){
                WorkerMain(std::move(token));
            });
    }
}

void DbThreadPool::PushJob(DbJob job)
{
    {
        WRITE_LOCK;
        _jobQueue.push(std::move(job));
    }
    _cv.notify_one();
}

void DbThreadPool::WorkerMain(std::stop_token token)
{
    DbConnection conn;
    if(!conn.Connect(_connectionString))
    {
        return;
    }
    std::vector<PreparedStatement> stmtCache;

    while(true)
    {
        DbJob job;
        {
            WRITE_LOCK;

            bool hasTask = _cv.wait(lockGuard_0, token, [this]() {
                return !_jobQueue.empty();
            });

            if(!hasTask)
            {
                break;
            }

            job = std::move(_jobQueue.front());
            _jobQueue.pop();
        }

        if(job) job(conn, stmtCache);
    }
}
