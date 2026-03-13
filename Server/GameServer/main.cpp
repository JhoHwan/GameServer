#include "pch.h"

#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <format>
#include <csignal>
#include "Service.h"
#include "GameSession.h"

#include <vector>

#include "Packet/ServerPacketHandler.h"
#include "Contents/Field/Field.h"

#include "NetCore.h"
#include "LogManager.h"
#include "Contents/Field/FieldManager.h"

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
	ServerPacketHandler::Init();
	if(!LogManager::Instance().Init(ELogLevel::Info))
	{
		cerr << "LogManager init Failed" << endl;
		return 0;
	}

	GFieldManager.Init();
    NetAddress address("127.0.0.1", 7777);
	NetCoreRef iocpCore = make_shared<NetCore>();

	ServerServiceRef service = make_shared<ServerService>(address, iocpCore, []()
		{
			return make_shared<GameSession>(); 
		}, 
		100);

	LOG_INFO(Default, "=======Server Start========");
	if(!service->Start())
	{
		LOG_INFO(Default, "Service Start Failed");
		return 0;
	}

	int32 workerCount = 4; //std::thread::hardware_concurrency();

    vector<thread> threads;
    for (int i = 0; i < workerCount; i++)
    {
        threads.emplace_back(WorkerMain, i+1, iocpCore);
    }
	LOG_INFO(Thread, "Worker Thread {} Start", workerCount);

	while(true)
	{
		string command;
		cin >> command;
	    if (command == "quit")
	    {
			LOG_INFO(Default, "=======Server Stop========");

		    service->CloseService();

            while(service->GetCurrentSessionCount() > 0)
            {
	            this_thread::sleep_for(std::chrono::milliseconds(100));
            }

	        this_thread::sleep_for(1s);

            GIsRunning = false;
	    	break;
	    }

		LOG_INFO(Command, "{}", command);

	}

	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

    return 0;
    
} 