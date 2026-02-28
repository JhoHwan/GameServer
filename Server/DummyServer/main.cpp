#include "pch.h"
#include <iostream>
#include <atomic>
#include "Service.h"
#include "Session.h"

#ifdef _WIN32
#include "IocpCore.h"
#endif

using namespace std;

// 접속한 클라이언트 수를 추적하기 위한 전역 변수
atomic<int32> GConnectedClientCount = 0;
atomic<bool> GIsRunning = true;

class DummyServerSession : public Session
{
public:
	virtual void OnConnected() override
	{
        int32 count = ++GConnectedClientCount;
		cout << "[Server] Client Connected! (Total: " << count << ")" << endl;
	}

	virtual int32 OnRecv(BYTE* buffer, int32 len) override
	{
		// 받은 데이터를 그대로 반환 (Echo)
		return len;
	}

	virtual void OnSend(int32 len) override 
	{ 
	}

	virtual void OnDisconnected() override 
	{ 
        int32 count = --GConnectedClientCount;
		cout << "[Server] Client Disconnected! (Total: " << count << ")" << endl;
	}
};

int main()
{
	cout << "=== Dummy Server Starting ===" << endl;

	NetAddress address(L"127.0.0.1", 7777);
#ifdef _WIN32
	NetCoreRef core = make_shared<IocpCore>();
#else
	NetCoreRef core = nullptr; // TODO: Epoll
#endif

    // ServerService 생성: 최대 1000명의 세션을 받을 수 있도록 설정
	ServerServiceRef service = make_shared<ServerService>(
		address,
		core,
		[]() { return make_shared<DummyServerSession>(); },
		1000 // maxSessionCount (Listener가 이 숫자만큼 AcceptEvent를 풀에 만들어 던짐)
	);

	if (service->Start() == false)
	{
		cout << "Failed to start ServerService" << endl;
		return 1;
	}

	cout << "DummyServer Listening on port 7777..." << endl;
	cout << "(Type 'quit' to shutdown gracefully)" << endl;

    // 워커 스레드 4개 가동
    vector<thread> threads;
	for (int32 i = 0; i < 4; i++)
	{
		threads.emplace_back([=]()
		{
			while (GIsRunning)
			{
				service->GetNetCore()->Dispatch(10);
			}
		});
	}

	// 메인 스레드는 명령어를 대기
	while (true)
	{
		wstring command;
		wcin >> command;
	    if (command == L"quit")
	    {
			cout << "Shutting down ServerService..." << endl;
			GIsRunning = false;
		    service->CloseService();
	    	break;
	    }
	}

	for (auto& t : threads)
	{
		t.join();
	}

	cout << "=== Dummy Server Terminated Safely ===" << endl;
    return 0;
}
