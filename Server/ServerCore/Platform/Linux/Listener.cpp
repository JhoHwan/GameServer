#include "pch.h"
#include "Listener.h"

#include <utility>
#include "SocketUtils.h"
#include "NetEvent.h"
#include "NetCore.h"
#include "Service.h"

/*--------------
	Listener
---------------*/

Listener::Listener()
{
	_nativeFlags = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
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

	RegisterAccept(nullptr);

	return true;
}

void Listener::CloseSocket()
{
	_service->GetNetCore()->UnRegister(shared_from_this());
	SocketUtils::Close(_socket);
	_socket = INVALID_SOCKET;

	_epollRef.reset();
}

HANDLE Listener::GetHandle()
{
	return _socket;
}

void Listener::Dispatch(NetEvent* netEvent, int32 numOfBytes)
{
	ProcessAccept(netEvent);
}

void Listener::RegisterAccept(AcceptEvent*)
{
	// Linux에서는 Reactor 패턴이므로 여기서 이벤트를 걸지 않고,
	// Epoll에서 이벤트가 통지되면 ProcessAccept에서 즉시 처리합니다.
}

void Listener::ProcessAccept(NetEvent* netEvent)
{
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
}
