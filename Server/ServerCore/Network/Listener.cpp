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
#ifndef _WIN32
	_nativeFlags = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
#endif
}

Listener::~Listener()
{
	SocketUtils::Close(_socket);
}

bool Listener::StartAccept(ServerServiceRef service)
{
	_service = std::move(service);
	if (_service == nullptr)
		return false;

	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
	{
		int error_code = SocketUtils::GetLastError();
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

#ifdef _WIN32
	const int32 acceptCount = _service->GetMaxSessionCount();
	for (int32 i = 0; i < acceptCount; i++)
	{
		auto* acceptEvent = new AcceptEvent();
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}
#else
	RegisterAccept(nullptr);
#endif
	return true;
}

void Listener::CloseSocket()
{
	_service->GetNetCore()->UnRegister(shared_from_this());
	SocketUtils::Close(_socket);

#ifdef _WIN32
	for (AcceptEvent* acceptEvent : _acceptEvents)
	{
		delete(acceptEvent);
	}
#else
	_epollRef.reset();
#endif
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(NetEvent* netEvent, int32 numOfBytes)
{
#ifdef _WIN32
	ASSERT_CRASH(netEvent->eventType == EventType::Accept);
#endif
	ProcessAccept(netEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
#ifdef _WIN32
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
#endif
}

void Listener::ProcessAccept(NetEvent* netEvent)
{
#ifdef _WIN32
	auto* acceptEvent = static_cast<AcceptEvent*>(netEvent);
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

#else
	while(true)
	{
		SOCKADDR_IN clientAddress;
		socklen_t clientAddressLength = sizeof(clientAddress);
		int clientSocket = accept4(_socket, reinterpret_cast<SOCKADDR*>(&clientAddress), &clientAddressLength, SOCK_NONBLOCK);
		if(clientSocket == INVALID_SOCKET)
		{
			if(SocketUtils::GetLastError() == EAGAIN || SocketUtils::GetLastError() == EWOULDBLOCK) break;
			perror("accept error");
			break;
		}

		SessionRef session = _service->CreateSession(clientSocket); //Epoll Register
		session->ProcessConnect();
	}
#endif
}