#include "pch.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <string>
#include "Service.h"
#include "GameSession.h"
#include "Util/Config/Config.h"

#include "DetourNavMesh.h"
#include <vector>

#include "Packet\ServerPacketHandler.h"
#include "Contents\Field.h"

#include <algorithm>

#pragma region MyRegion
/*
//#include <random>
//std::atomic<int64_t> g_TotalExecuted = 0;
//std::atomic<int64_t> g_TotalPush = 0;
//
//vector<shared_ptr<JobQueue>> g_Players;
//int main()
//{
//    IocpCoreRef iocpCore = make_shared<IocpCore>();
//    // 1. 플레이어(JobQueue) 100명 생성
//    for (int i = 0; i < 100; i++)
//    {
//        g_Players.push_back(make_shared<JobQueue>());
//    }
//    const int TOTAL_JOBS = 100000000; // 100만 개
//    const int WORKER_THREAD = 16; // 100만 개
//
//    vector<thread> workers;
//    for (int i = 0; i < WORKER_THREAD; i++)
//    {
//        workers.push_back(thread([iocpCore]()
//            			{
//                            static thread_local std::mt19937 gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
//                            std::uniform_int_distribution<> dist(0, g_Players.size() - 1);
//            				while (true)
//            				{
//                                if (g_TotalPush.load() < TOTAL_JOBS)
//                                {
//                                    auto ioStart = chrono::steady_clock::now();
//                                    while (true)
//                                    {
//                                        auto now = chrono::steady_clock::now();
//                                        auto duration = chrono::duration_cast<chrono::milliseconds>(now - ioStart).count();
//
//                                        // 5ms 지났으면 루프 탈출 (Consumer 하러 가야 함)
//                                        if (duration >= 5) break;
//
//                                        // ====================================================
//                                        // [핵심 수정] 티켓 먼저 끊기 (Atomic)
//                                        // ====================================================
//                                        long long myTicket = g_TotalPush.fetch_add(1);
//
//                                        // 티켓 번호가 정원 초과면? -> Job 생성 포기
//                                        if (myTicket >= TOTAL_JOBS)
//                                        {
//                                            // 주의: 여기서 fetch_add를 취소할 필요는 없음.
//                                            // 그냥 Job 생성만 안 하면 Executed 개수는 정확히 맞음.
//                                            break;
//                                        }
//
//                                        // --- 유효한 티켓을 가진 자만 Job 생성 ---
//                                        int playerIdx = dist(gen); // (O) 빠름
//
//                                        auto job = make_shared<Job>([=]() {
//                                            g_TotalExecuted.fetch_add(1);
//                                            for (int i = 0; i < 1000; i++) { int a = 1000 * 1000; }
//                                            });
//
//                                        // 여기서 g_TotalPush.fetch_add(1) 하면 절대 안 됨! (위에서 이미 했음)
//                                        g_Players[playerIdx]->Push(job);
//                                    }
//                                }
//
//            					auto start = chrono::steady_clock::now();
//            					while (!LJobQueue.empty())
//            					{
//            						auto now = chrono::steady_clock::now();
//            						auto duration = chrono::duration_cast<chrono::milliseconds>(now - start).count();
//            						if (duration >= 10)
//            						{
//            							while (!LJobQueue.empty())
//            							{
//            								GGlobalJobQueue.push(LJobQueue.front());
//            								LJobQueue.pop();
//            							}
//            							break;
//            						}
//
//            						auto jobQueue = LJobQueue.front();
//            						LJobQueue.pop();
//
//
//            						jobQueue->Execute(static_cast<int32>(10 - duration));
//            					}
//
//            					while(true)
//            					{
//            						auto now = chrono::steady_clock::now();
//            						auto duration = chrono::duration_cast<chrono::milliseconds>(now - start).count();
//            						if (duration >= 10) break;
//
//                                    JobQueueRef jobQueue;
//                                    if (!GGlobalJobQueue.try_pop(jobQueue)) break;
//            						if (!jobQueue) break;
//
//            						int32 remainTime = static_cast<int32>(10 - duration);
//            						if (remainTime < 0) remainTime = 0;
//
//            						jobQueue->Execute(remainTime);
//            					}
//            				}
//            			}));
//    }
//
//    // 3. 스트레스 시작 (생산자)
//    cout << "Start Pushing " << TOTAL_JOBS << " jobs..." << endl;
//
//    auto start = chrono::steady_clock::now();
//
//    while (g_TotalPush.load() < TOTAL_JOBS)
//    {
//        this_thread::sleep_for(std::chrono::milliseconds(10));
//    }
//    cout << "Push Finished. Waiting for Workers..." << endl;
//
//    // 5. 모든 작업이 끝날 때까지 대기 (Polling)
//    while (g_TotalExecuted.load() < TOTAL_JOBS)
//    {
//        this_thread::sleep_for(std::chrono::milliseconds(10));
//    }
//
//    auto end = chrono::steady_clock::now();
//    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
//
//    // 6. 결과 리포트
//    cout << "========================================" << endl;
//    cout << "Worker Thread  : " << WORKER_THREAD << endl;
//    cout << "Total Scheduled: " << TOTAL_JOBS << endl;
//    cout << "Total Executed : " << g_TotalExecuted.load() << endl; // 이 숫자가 다르면 버그 있음!
//    cout << "Elapsed Time   : " << duration << "ms" << endl;
//    cout << "Throughput     : " << (int)((TOTAL_JOBS / (double)duration) * 1000) << " jobs/sec" << endl;
//    cout << "========================================" << endl;
//
//    // 워커 스레드는 무한루프라 join 못하니 강제 종료 (테스트니까)
//    return 0;
//}
*/

#pragma endregion

void WorkerMain(INetCoreRef iocpCore)
{
    while (true)
    {
        // IOCP(GQCSEx) 처리
        iocpCore->Dispatch(5);

        {
            auto now = GetTickCount64();
            GJobTimer.Distribute(now);
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

            auto jobQueue = LJobQueue.front();
            LJobQueue.pop();


            jobQueue->Execute(static_cast<int32>(10 - duration));
        }

        while (true)
        {
            auto now = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(now - start).count();
            if (duration >= 10) break;

            JobQueueRef jobQueue;
            if (!GGlobalJobQueue.try_dequeue(jobQueue)) break;
            if (!jobQueue) break;

            int32 remainTime = static_cast<int32>(10 - duration);
            if (remainTime < 0) remainTime = 0;

            jobQueue->Execute(remainTime);
        }
    }
}

int main()
{
	GFieldManager.Create(0);

	ServerPacketHandler::Init();


    NetAddress address(L"127.0.0.1", 7777);
    NetAddress dbAddress(L"127.0.0.1", 12345);
	INetCoreRef iocpCore = make_shared<IocpCore>();

	ServerServiceRef service = make_shared<ServerService>(address, iocpCore, []() 
		{
			return make_shared<GameSession>(); 
		}, 
		100);
	service->Start();
    vector<thread> threads(4);
    for (int i = 0; i < 4; i++)
    {
        threads.emplace_back(WorkerMain, iocpCore);
    }

	while(true)
	{
		wstring command;
		wcin >> command;
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