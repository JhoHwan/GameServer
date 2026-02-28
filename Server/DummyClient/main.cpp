#include "pch.h"
#include <iostream>
#include <atomic>
#include "Service.h"
#include "Session.h"

#ifdef _WIN32
#include "IocpCore.h"
#endif

using namespace std;

atomic<bool> GIsRunning = true;

class DummySession : public Session
{
public:
	virtual void OnConnected() override
	{
		cout << "Connected To Server!" << endl;
	}

	virtual int32 OnRecv(BYTE* buffer, int32 len) override
	{
		return len;
	}

	virtual void OnSend(int32 len) override 
	{ 
	}

	virtual void OnDisconnected() override 
	{ 
		cout << "Disconnected!" << endl;
	}
};

int main()
{
	this_thread::sleep_for(1s); // 서버가 켜질 시간 대기

	NetAddress address(L"127.0.0.1", 7777);
#ifdef _WIN32
	NetCoreRef core = make_shared<IocpCore>();
#else
	NetCoreRef core = nullptr; // TODO: Epoll
#endif

    // ClientService 생성: 100명의 더미 클라이언트가 동시에 접속을 시도하게 함
	ClientServiceRef service = make_shared<ClientService>(
		address,
		core,
		[]() { return make_shared<DummySession>(); },
		100 // maxSessionCount (더미 수)
	);

	if (service->Start() == false)
	{
		cout << "Failed to start ClientService" << endl;
		return 1;
	}

	cout << "DummyClient started. Connecting to server..." << endl;

    // 워커 스레드 2개 가동
    vector<thread> threads;
	for (int32 i = 0; i < 2; i++)
	{
		threads.emplace_back([=]()
		{
			while (GIsRunning)
			{
				service->GetNetCore()->Dispatch(10);
			}
		});
	}

	while (true)
	{
		wstring command;
		wcin >> command;
	    if (command == L"quit")
	    {
			GIsRunning = false;
		    service->CloseService();
	    	break;
	    }
	}

	for (auto& t : threads)
	{
		t.join();
	}

    return 0;
}
