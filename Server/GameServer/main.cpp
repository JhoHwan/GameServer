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
#include "Util/MonitorManager.h"
#include "Util/Time.h"

std::atomic<bool> GIsRunning{true};

void WorkerMain(uint32 id, const NetCoreRef& netCore)
{
	LThreadId = id;
	while (GIsRunning)
	{
		netCore->Dispatch(5);
		{
			auto now = GetTickCount64();
			LJobTimer.Distribute(now);
		}

		// Job처리
		int32 processCount = 0;
		auto start = GetTickCount64();
		while(true)
		{
			auto now = GetTickCount64();
			if(processCount >= 64)
			{
				processCount = 0;
				if(now - start >= 20)
				{
					break;
				}
			}

			JobQueueRef jobQueue;
			if(LJobQueue.empty())
			{
				if(!GGlobalJobQueue.try_dequeue(jobQueue)) break;
			}
			else
			{
				jobQueue = LJobQueue.front();
				LJobQueue.pop();
			}

			processCount += jobQueue->Execute();
		}

		if(!LSendSessionList.empty())
		{
			for(auto& session : LSendSessionList)
			{
				session->FlushSend();
			}
			LSendSessionList.clear();
		}
	}
}

int main()
{
	ServerPacketHandler::Init();

	if(!LogManager::Instance().Init(ELogLevel::Debug))
	{
		cerr << "LogManager init Failed" << endl;
		return 0;
	}
	GMonitorManager.Init(5s);
	GFieldManager.Init();

    NetAddress address("0.0.0.0", 7777);
	NetCoreRef iocpCore = make_shared<NetCore>();

	ServerServiceRef service = make_shared<ServerService>(address, iocpCore, []()
		{
			return make_shared<GameSession>(); 
		}, 
		10000);

	LOG_INFO(Default, "=======Server Start========");
	Time::InitServerStartTime();
	if(!service->Start())
	{
		LOG_INFO(Default, "Service Start Failed");
		return 0;
	}

	int32 workerCount = 4;//std::thread::hardware_concurrency();

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