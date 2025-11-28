#include "pch.h"
#include "Session.h"
#include "SocketUtil.h"
#include "IOCPServer.h"
#include "SendBuffer.h"


Session::Session() 
	: _recvEvent(), _disConnectEvent(), _recvBuffer(4096)
{ 
	_isConnect.store(false);
	_sendRegistered.store(false);
}

Session::~Session()
{
	cout << "Release Session" << endl;
	SocketUtil::CloseSocket(_socket);
}

void Session::CreateSocket()
{
	_socket = SocketUtil::CreateSocket();
	if (_socket == INVALID_SOCKET)
	{
		auto errCode = WSAGetLastError();
		cout << errCode;
	}
}

void Session::Dispatch(class IOCPEvent* iocpEvent, int32 numOfBytes)
{
	EventType eventType = iocpEvent->GetEventType();
	switch (eventType)
	{
	case EventType::RegisterSend:
		RegisterSend();
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	default:
		Disconnect();
		break;
	}
}

void Session::Send(shared_ptr<SendBuffer> sendBuffer)
{
	if (_isConnect.load() == false)
		return;

	_sendQueue.try_enqueue(sendBuffer);

	if (_sendRegistered.exchange(true) == false)
	{
		_registerSendEvent.Init();
		_registerSendEvent.owner = shared_from_this();

		PostQueuedCompletionStatus(
			_iocpHandle,
			0,
			0,
			reinterpret_cast<LPOVERLAPPED>(&_registerSendEvent)
		);
	}
}

void Session::RegisterSend()
{
	if (_isConnect.load() == false)
		return;

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this();

	uint32 queueSize = _sendQueue.size_approx();
	if (queueSize == 0)
	{
		cout << "Queue Size 0" << endl;
		return;
	}

	uint32 writeSize = 0;
	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(queueSize);

	for(int32 i = 0; i < queueSize; i++)
	{
		shared_ptr<SendBuffer> sendBuffer;
		_sendQueue.try_dequeue(sendBuffer);

		if (writeSize + sendBuffer->WriteSize() > MAX_SEND_SIZE)
		{
			if (writeSize == 0)
			{
				cout << "SendBuffer Size Overflow" << endl;
				Disconnect();
				return;
			}
			break;
		}

		writeSize += sendBuffer->WriteSize();
		_sendEvent.sendBuffers.push_back(sendBuffer);

		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	if (wsaBufs.size() == 0) RegisterSend();

	DWORD lpNumberOfBytesSent;
	if (SOCKET_ERROR == ::WSASend(GetSocket(), wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), &lpNumberOfBytesSent, 0, reinterpret_cast<LPOVERLAPPED>(&_sendEvent), nullptr))
	{
		auto errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			// TODO : Error Logging
			if (errCode != WSAECONNRESET)
			{
				cout << "WSASend Error : " << errCode << endl;
				Disconnect();

			}
			_sendEvent.owner.reset(); // Release-Ref
			_sendEvent.sendBuffers.clear();
			_sendRegistered.store(false);
		}
	}
}

void Session::ProcessSend(uint32 sentBytes)
{
	_sendEvent.owner.reset();
	_sendEvent.sendBuffers.clear();


	if (sentBytes == 0)
	{
		Disconnect();
		return;
	}

	OnSend(sentBytes);

	{
		if (_sendQueue.size_approx() == 0)
		{
			_sendRegistered.store(false);
			return;
		}
	}
	
	PostQueuedCompletionStatus(
		_iocpHandle,
		0,
		0,
		reinterpret_cast<LPOVERLAPPED>(&_registerSendEvent)
	);
}

void Session::RegisterRecv()
{
	if (_isConnect.load() == false)
		return;

	_recvEvent.owner = shared_from_this();

	WSABUF wsaBuf{};
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.Capacity();

	DWORD lpNumberOfBytesRecvd = 0;
	DWORD flag = 0;
	if (SOCKET_ERROR == ::WSARecv(GetSocket(), &wsaBuf, 1, &lpNumberOfBytesRecvd, &flag, reinterpret_cast<LPOVERLAPPED>(&_recvEvent), nullptr))
	{
		auto errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			//TODO : Error Log
			cout << "WSARecv Error" << errCode << endl;
			return;
		}
	}
}

void Session::ProcessRecv(uint32 recvBytes)
{
	_recvEvent.owner.reset(); // Release-Ref
	if (_recvBuffer.OnWirte(recvBytes) == false)
	{
		cout << "RecvBuffer Overflow!" << endl;
		RegisterRecv();
		return;
	}

	if (recvBytes == 0)
	{
		Disconnect();
		return;
	}

	int32 processLen = 0;
	BYTE* buffer = _recvBuffer.ReadPos();
	while (true)
	{
		int32 dataSize = _recvBuffer.DataSize() - processLen;

		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		if (dataSize < header.size)
			break;

		// 패킷 조립 성공
		OnRecv(&buffer[processLen], header.size);

		_recvBuffer.OnRead(header.size);

		processLen += header.size;
	}

	_recvBuffer.Clean();

	RegisterRecv();
}

void Session::Connect()
{
}

void Session::ProcessConnect()
{
	_isConnect.store(true);

	OnConnected();

	RegisterRecv();
}

void Session::Disconnect()
{
	_isConnect.store(false);

	RegisterDisconnect();
}

void Session::RegisterDisconnect()
{
	auto DisconnectEx = SocketUtil::GetDisconnectEx();
	_disConnectEvent.Init();
	_disConnectEvent.owner = shared_from_this();

	if (SOCKET_ERROR == DisconnectEx(_socket, reinterpret_cast<LPOVERLAPPED>(&_disConnectEvent), TF_REUSE_SOCKET, 0))
	{
		auto errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			_disConnectEvent.owner.reset();
			cout << "DisconnectEx Error" << errCode << endl;
			return;
		}
	}
}

void Session::ProcessDisconnect()
{
	::shutdown(_socket, SD_BOTH);
	::closesocket(_socket);

	_disConnectEvent.owner.reset();
	OnDisconnected();

	if (_server == nullptr) return;
	_server->DeleteSession(GetSharedPtr());
}

void Session::OnConnected()
{
	auto ip = _address.GetIpAddress();
	wprintf(L"Connect : %s\n", ip.c_str());
}

void Session::OnDisconnected()
{
	auto ip = _address.GetIpAddress();
	wprintf(L"Disconnect : %s\n", ip.c_str());
}

void Session::OnSend(uint32 sentBytes)
{
	//wprintf(L"Send : %d\n", sentBytes);
}

void Session::OnRecv(BYTE* buffer, int32 len)
{
}
