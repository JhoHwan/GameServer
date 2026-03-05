#include "pch.h"

#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include "Service.h"
#include "GameSession.h"

#include <vector>

#include "Packet/ServerPacketHandler.h"
#include "Contents/Field.h"

#include "NetCore.h"
#include "LogManager.h"

std::atomic<bool> GIsRunning{true};

void WorkerMain(uint32 id, const NetCoreRef& netCore)
{
	LThreadId = id;
    while (GIsRunning)
    {
        // IOCP(GQCSEx) 처리
        netCore->Dispatch(5);

        {
            auto now = GetTickCount64();
            LJobTimer.Distribute(now);
        }

        // Job처리
        auto start = chrono::steady_clock::now();

        while (!LJobQueue.empty())
        {
            auto now = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(now - start).count();
            if (duration >= 10)
            {
                while (!LJobQueue.empty())
                {
                    GGlobalJobQueue.enqueue(LJobQueue.front());
                    LJobQueue.pop();
                }
                break;
            }

            JobQueueRef jobQueue = LJobQueue.front();
            LJobQueue.pop();

            jobQueue->Execute(64);
        }

        while (true)
        {
            auto now = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(now - start).count();
            if (duration >= 10) break;

            JobQueueRef jobQueue;
            if (!GGlobalJobQueue.try_dequeue(jobQueue)) break;
            if (!jobQueue) break;

            jobQueue->Execute(64);
        }
    }
}

int main()
{
	GFieldManager.Create(0);

	ServerPacketHandler::Init();
	LogManager::Instance().Init();


    NetAddress address("127.0.0.1", 7777);
	NetCoreRef iocpCore = make_shared<NetCore>();

	ServerServiceRef service = make_shared<ServerService>(address, iocpCore, []() 
		{
			return make_shared<GameSession>(); 
		}, 
		100);
	service->Start();
    vector<thread> threads;
    for (int i = 0; i < 4; i++)
    {
        threads.emplace_back(WorkerMain, i, iocpCore);
    }

	while(true)
	{
		string command;
		cin >> command;
	    if (command == "quit")
	    {
		    service->CloseService();

            while(service->GetCurrentSessionCount() > 0)
            {
	            this_thread::sleep_for(std::chrono::milliseconds(100));
            }

	        this_thread::sleep_for(1s);

            GIsRunning = false;
	    	break;
	    }

		LogManager::Instance().WriteLog(ELogLevel::Log, std::move(command));

	}

	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

    //auto navMesh = NavMeshLoader::LoadNavMeshFromBin("Map.bin");
    //dtNavMeshQuery* navQuery = dtAllocNavMeshQuery();
    //dtStatus Status = navQuery->init(navMesh, 2048);
    //if (dtStatusFailed(Status))
    //{
    //    dtFreeNavMeshQuery(navQuery);
    //    return -1;
    //}

    //dtReal StartPos[3]{ 0, 302, 0 };
    //dtReal EndPos[3]{ 505, 92.15, 1265};

    //NavMeshLoader::TestFindPath(navQuery, StartPos, EndPos);

    return 0;
    
} 