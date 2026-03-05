#include "NetCore.h"
#include "SocketUtils.h"
#include "Service.h"

/*--------------
	Session
---------------*/

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	_nativeFlags = EPOLLIN | EPOLLET | EPOLLONESHOT;
}

Session::~Session()
{
	//cout << "Session Free" << endl;
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;

	_sendQueue.enqueue(std::move(sendBuffer));
	if (_sendRegistered.exchange(true) == false)
		registerSend = true;
	
	if (registerSend)
		RegisterSend();
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const char* cause)
{
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	cout << "Disconnect : " << cause << endl;

	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(NetEvent* netEvent, const int32 numOfBytes)
{
	SessionRef sessionRef = GetSessionRef();
	const auto flags = netEvent->eventFlags;

	if(flags & (EPOLLERR | EPOLLHUP))
	{
		auto err = SocketUtils::GetLastError();
		Disconnect("Error");
		return;
	}

	if(flags & (EPOLLIN))
	{
		ProcessRecv(numOfBytes);
	}

	if(flags & (EPOLLOUT))
	{
		if(IsConnected()) FlushSend();
		else ProcessConnect();
	}

	if(flags & EPOLLRDHUP)
	{
		Disconnect("EOF");
		return;
	}

	if(IsConnected())
	{
		uint32 nextEvents = EPOLLIN;

		if(_sendRegistered.load() == true) nextEvents |= EPOLLOUT;
		GetService()->GetNetCore()->Update(shared_from_this(), nextEvents);
	}
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::BindAnyAddress(_socket, 0) == false)
		return false;

	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();

	GetService()->GetNetCore()->Update(shared_from_this(), EPOLLIN | EPOLLOUT);
	int res = ::connect(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr));
	if(res == SOCKET_ERROR)
	{
		if(SocketUtils::GetLastError() != EINPROGRESS)
		{
			return false;
		}
	}
	return true;
}

bool Session::RegisterDisconnect()
{
	ProcessDisconnect();
	return true;
}

void Session::RegisterRecv()
{
	// Linux에서는 Reactor 패턴이므로 여기서 별도의 I/O를 걸지 않습니다.
}

void Session::FlushSend()
{
	int32 totalSentBytes = 0;
	while(true)
	{
		constexpr int MAX_IOV = 64;

		_sendQueue.try_dequeue_bulk(back_inserter(_pendingSendQueue), MAX_IOV);

		if (_pendingSendQueue.empty())
		{
			_sendRegistered.store(false);

			if (_sendQueue.size_approx() > 0)
			{
				if (_sendRegistered.exchange(true) == false)
					continue;
			}
			break;
		}

		iovec iovs[MAX_IOV];
		int32 iovCount = 0;
		size_t totalBytes = 0;

		auto it = _pendingSendQueue.begin();
		while (it != _pendingSendQueue.end() && iovCount < MAX_IOV)
		{
			auto& buffer = *it;
			int32 offset = (iovCount == 0) ? _sendOffset : 0;

			iovs[iovCount].iov_base = buffer->Buffer() + offset;
			iovs[iovCount].iov_len = buffer->WriteSize() - offset;

			totalBytes += iovs[iovCount].iov_len;
			++iovCount;
			++it;
		}

		ssize_t sentBytes = ::writev(_socket, iovs, iovCount);
		if(sentBytes == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK) break;
			Disconnect("Writev Error");
			return;
		}

		totalSentBytes += static_cast<int32>(sentBytes);


		auto bytesToRemove = static_cast<int32>(sentBytes);

		while (bytesToRemove > 0 && _pendingSendQueue.empty() == false)
		{
			const auto& buffer = _pendingSendQueue.front();
			int32 dataInBuf = buffer->WriteSize() - _sendOffset;
			if(bytesToRemove >= dataInBuf)
			{
				bytesToRemove -= dataInBuf;
				_pendingSendQueue.pop_front();
				_sendOffset = 0;
			}
			else
			{
				_sendOffset += bytesToRemove;
				bytesToRemove = 0;
			}
		}

		if(_pendingSendQueue.empty() || sentBytes < totalBytes) break;
	}
	if(totalSentBytes > 0)
		OnSend(totalSentBytes);
}

void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	FlushSend();
	if(_sendRegistered.load())
		GetService()->GetNetCore()->Update(shared_from_this(), EPOLLIN | EPOLLOUT);
}

void Session::ProcessConnect()
{
	GetService()->AddSession(GetSessionRef());

	_connected.store(true);

	OnConnected();

	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	OnDisconnected();

	GetService()->GetNetCore()->UnRegister(shared_from_this());
	SocketUtils::Close(_socket);

	GetService()->ReleaseSession(GetSessionRef());
	
	_epollRef.reset();
}

void Session::ProcessRecv(int32 numOfBytes)
{
	while (true)
	{
		ssize_t recvBytes = recv(_socket, _recvBuffer.WritePos(), _recvBuffer.FreeSize(), 0);
		if (recvBytes == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) break;
			Disconnect("Error");
			return;
		}
		else if (recvBytes == 0)
		{
			Disconnect("Recv 0");
			return;
		}
		else
		{
			if(_recvBuffer.OnWrite(recvBytes) == false)
			{
				Disconnect("OnWrite Overflow");
				return;
			}
		}
	}

	int32 dataSize = _recvBuffer.DataSize();
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);
	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect("OnRead Overflow");
		return;
	}
	
	_recvBuffer.Clean();

	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
	// Linux의 Epoll Dispatch는 ProcessSend를 직접 호출하지 않습니다.
}

void Session::HandleError(int32 errorCode)
{
	cout << "Handle Error : " << errorCode << endl;
}

/*-----------------
	PacketSession
------------------*/

int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		if (dataSize < header.size)
			break;

		OnRecvPacket(&buffer[processLen], header.size);

		processLen += header.size;
	}

	return processLen;
}
