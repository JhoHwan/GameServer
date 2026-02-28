#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

#ifdef _WIN32
	#include "Listener_Win.h"
#else
#endif


/*--------------
	Listener
---------------*/

Listener::Listener()
{
	_impl = ListenerImpl::CreateListenerImpl(this);
}

Listener::~Listener()
{
	SocketUtils::Close(_socket);
}

bool Listener::StartAccept(ServerServiceRef service)
{
	_service = service;
	if (_service == nullptr)
		return false;

	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
	{
		return false;
	}

	if (_service->GetNetCore()->Register(shared_from_this()) == false)
	{
		SocketUtils::Close(_socket);
		return false;
	}

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
	{
		SocketUtils::Close(_socket);
		return false;
	}

	if (SocketUtils::SetLinger(_socket, 0, 0) == false)
	{
		SocketUtils::Close(_socket);
		return false;
	}

	if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false)
	{
		SocketUtils::Close(_socket);
		return false;
	}

	if (SocketUtils::Listen(_socket) == false)
	{
		SocketUtils::Close(_socket);
		return false;
	}

	if (_impl->StartAccept(service) == false)
	{
		SocketUtils::Close(_socket);
		return false;
	}

	return true;
}

void Listener::CloseSocket()
{
	SocketUtils::Close(_socket);
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(NetEvent* netEvent, int32 numOfBytes)
{
	_impl->Dispatch(netEvent, numOfBytes);
}