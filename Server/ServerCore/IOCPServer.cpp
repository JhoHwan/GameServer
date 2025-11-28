#include "pch.h"
#include <format>
#include "type.h"
#include "IOCPServer.h"
#include "IOCPEvent.h"
#include "SendBuffer.h"

IOCPServer::IOCPServer(NetAddress address, SessionFactory sessionFactory) 
    : _address(address), _sessionFactory(sessionFactory)
{
}

IOCPServer::~IOCPServer()
{
    _listener.reset();
}

shared_ptr<Session> IOCPServer::CreateSession()
{
    auto session = _sessionFactory();

    session->CreateSocket();

    if (session->GetSocket() == INVALID_SOCKET)
    {
        auto errCode = WSAGetLastError();
        cout << errCode;
    }

    // 소캣을 IOCP 핸들에 연결
    if (_iocpCore.RegisterSocket(session->GetSocket()) == false)
    {
        cout << "RegisterSocket Error" << endl;
        return nullptr;
    }
    return session;
}

void IOCPServer::BroadCast(shared_ptr<SendBuffer> sendBuffer)
{
    lock_guard<mutex> lock(_lock);
    if (_sessions.size() == 0) return;
    for (const auto& session : _sessions)
    {
        session->Send(sendBuffer);
    }
}

bool IOCPServer::Start()
{
	_isRunning = true;
	for (int i = 0; i < 16; i++)
	{
		_iocpThreads.emplace_back(thread([this]() 
            { 
                while (_isRunning)
                {
                    Dispatch(100);
                }
            })
        );
        wstring name = std::format(L"IOCP Thread {}", i) ;
        ::SetThreadDescription(_iocpThreads[i].native_handle(), name.data());
		_iocpThreads[i].detach();
	}

    _listener = make_shared<Listener>(10);

    _listener->StartAccept(shared_from_this());

    return true;
}

bool IOCPServer::Stop()
{
    _isRunning = false;

    ::WSACleanup();

    for (int i = 0; i < 4; i++)
    {
		_iocpThreads[i].join();
    }

    return true;
}

void IOCPServer::DispatchIocpEvent(uint16 time)
{
    _iocpCore.Dispatch(time);
}

void IOCPServer::DispatchJob(uint16 time)
{
    auto endTime = chrono::system_clock::now() + chrono::milliseconds(time);

    while (true)
    {
        auto jobQueue = GJobManager->Pop();
        if (jobQueue == nullptr) return;

        if (jobQueue->TryExcute() == false)
        {
            GJobManager->Push(jobQueue); // 이미 다른 쓰레드에서 작업중이면 다시 반환
        }

        auto curTime = chrono::system_clock::now();
        if (curTime >= endTime) // 한 개의 JobQueue작업 후 시간이 초과했으면 리턴
        {
            return;
        }
    }
}

void IOCPServer::Dispatch(uint16 iocpDispatchTime)
{
    DispatchIocpEvent(iocpDispatchTime);

    //DispatchJob(jobDispatchTime);
}

void IOCPServer::AddSession(shared_ptr<Session> session)
{
    lock_guard<mutex> lock(_lock);
    _sessions.insert(session);
    session->SetServer(shared_from_this());
    session->SetIOCPHandle(_iocpCore.GetHandle());
}

void IOCPServer::DeleteSession(std::shared_ptr<Session> session)
{
    lock_guard<mutex> lock(_lock);

    _sessions.erase(session);

    // 세션 객체의 레퍼런스 감소
    session.reset();
}