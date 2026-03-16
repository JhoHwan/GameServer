#include "pch.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

#include "NetCore.h"
#include "Service.h"
#include "Session.h"
#include "DummySession.h"
#include "Packet/ClientPacketHandler.h"

using namespace std;

atomic<bool> GIsRunning = true;
atomic<int32> GConnectedCount = 0;

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
    ClientPacketHandler::Init();

	this_thread::sleep_for(1s); 

	int32 sessionCount = 0;
	cout << "Enter Dummy Session Count: ";
	cin >> sessionCount;

	if (sessionCount <= 0)
	{
		cout << "Invalid Session Count" << endl;
		return 0;
	}

	NetAddress address("127.0.0.1", 7777);
	NetCoreRef core = make_shared<NetCore>();

    // 입력받은 수만큼 더미가 생성되도록 설정
	ClientServiceRef service = make_shared<ClientService>(
		address,
		core,
		[]() { return make_shared<DummySession>(); },
		sessionCount 
	);

	if (service->Start() == false) return 1;

	cout << "Stress Test Started (" << sessionCount << " Dummies with JobQueue)..." << endl;

    vector<thread> threads;
	for (int32 i = 0; i < 8; i++) // 워커 쓰레드 4개
	{
		threads.emplace_back(WorkerMain, i + 1, core);
	}

    // 상태 보고용 쓰레드
    threads.emplace_back([=]() {
        while (GIsRunning) {
            this_thread::sleep_for(2s);
            cout << "Current Connected Dummies: " << GConnectedCount.load() << endl;
        }
    });

	while (true)
	{
		string command;
		cin >> command;
		if (command == "quit")
		{
			GIsRunning = false;
			break;
		}
	}

	for (auto& t : threads) t.join();

    return 0;
}
