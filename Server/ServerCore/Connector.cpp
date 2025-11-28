#include "pch.h"
#include "Connector.h"

Connector::Connector()
{
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), OUT & wsaData))
	{
		cout << "WSAStartup Error" << endl;
		return;
	}
}

Connector::~Connector()
{
	WSACleanup();
}

void Connector::Connect(wstring ip, uint16 port, shared_ptr<Session> session)
{
	ConnectEvent* connectEvent = new ConnectEvent();
	connectEvent->Init();
	connectEvent->owner = shared_from_this();
	connectEvent->session = session;

	RegisterConnect(ip, port, connectEvent);
}

void Connector::Dispatch(IOCPEvent* iocpEvent, int32 numOfBytes)
{
	if (iocpEvent->GetEventType() != EventType::Connect)
	{
		return;
	}

	auto connectEvent = static_cast<ConnectEvent*>(iocpEvent);
	connectEvent->session->ProcessConnect();

	connectEvent->session.reset();
	delete connectEvent;
}

bool Connector::RegisterConnect(wstring ip, uint16 port, ConnectEvent* connectEvent)
{
	const SOCKET& socket = connectEvent->session->GetSocket();

	if (SocketUtil::BindAnyAddress(socket, 0) == false)
	{
		auto errCode = WSAGetLastError();

		if (errCode != WSAEINVAL)
		{
			return false;
		}
	}

	if (SocketUtil::SetReuseAddr(socket) == false)
	{
		return false;
	}

	DWORD numOfBytes = 0;
	NetAddress addr(ip, port);
	auto addr_in = addr.GetAddress();

	auto ConnectEx = SocketUtil::GetConnectEx();
	if (false == ConnectEx(socket, reinterpret_cast<SOCKADDR*>(&addr_in), sizeof(addr_in), nullptr, 0, &numOfBytes, reinterpret_cast<OVERLAPPED*>(connectEvent)))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			connectEvent->owner.reset(); // RELEASE_REF
			connectEvent->session.reset(); // RELEASE_REF
			delete connectEvent;
			return false;
		}
	}

	return true;
}