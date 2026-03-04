#include "pch.h"
#include "Listener.h"

#include <utility>
#include "SocketUtils.h"
#include "NetEvent.h"
#include "NetCore.h"
#include "Session.h"
#include "Service.h"

/*--------------
	Listener
---------------*/

Listener::Listener()
{
}

Listener::~Listener()
{
	cout << "Listener::~Listener()" << endl;
}

bool Listener::StartAccept(ServerServiceRef service)
{
	_service = std::move(service);
	if (_service == nullptr)
		return false;

	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
	{
		return false;
	}

	if (_service->GetNetCore()->Register(shared_from_this()) == false)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::SetLinger(_socket, 0, 0) == false)
		return false;

	if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false)
		return false;

	if (SocketUtils::Listen(_socket) == false)
		return false;

	const int32 acceptCount = _service->GetMaxSessionCount();
	for (int32 i = 0; i < acceptCount; i++)
	{
		auto* acceptEvent = new AcceptEvent();
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}

	return true;
}

void Listener::CloseSocket()
{
	_service->GetNetCore()->UnRegister(shared_from_this());
	SocketUtils::Close(_socket);
	_socket = INVALID_SOCKET;
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(NetEvent* netEvent, int32 numOfBytes)
{
	ASSERT_CRASH(netEvent->eventType == EventType::Accept);
	ProcessAccept(netEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	if(_socket == INVALID_SOCKET)
	{
		delete acceptEvent;
		return;
	}

	SessionRef session = _service->CreateSession(SocketUtils::CreateSocket()); // Register IOCP

	acceptEvent->Init();
	acceptEvent->session = session;

	DWORD bytesReceived = 0;
	if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), session->_recvBuffer.WritePos(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 일단 다시 Accept 걸어준다
			RegisterAccept(acceptEvent);
		}
	}
}

void Listener::ProcessAccept(NetEvent* netEvent)
{
	auto* acceptEvent = static_cast<AcceptEvent*>(netEvent);

	if(_socket == INVALID_SOCKET)
	{
		delete acceptEvent;
		return;
	}

	const SessionRef session = acceptEvent->session;

	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	session->SetNetAddress(NetAddress(sockAddress));
	session->ProcessConnect();
	RegisterAccept(acceptEvent);
}
