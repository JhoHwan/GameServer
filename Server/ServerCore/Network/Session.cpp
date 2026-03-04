#include "NetCore.h"
#include "SocketUtils.h"
#include "Service.h"

/*--------------
	Session
---------------*/

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	//_socket = SocketUtils::CreateSocket();
#ifndef _WIN32
	_nativeFlags = EPOLLIN | EPOLLET | EPOLLONESHOT;
#endif
}

Session::~Session()
{
	cout << "Session Free" << endl;
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;

	// 현재 RegisterSend가 걸리지 않은 상태라면, 걸어준다
	{
		WRITE_LOCK;

		_sendQueue.push(std::move(sendBuffer));

		if (_sendRegistered.exchange(true) == false)
			registerSend = true;
	}
	
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
#ifdef _WIN32
	switch (netEvent->eventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
#else
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

#endif
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::BindAnyAddress(_socket, 0/*남는거*/) == false)
		return false;

	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();

#ifdef _WIN32
	_connectEvent.Init();
	_connectEvent.owner = shared_from_this(); // ADD_REF

	DWORD numOfBytes = 0;
	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}
#else
	GetService()->GetNetCore()->Update(shared_from_this(), EPOLLIN | EPOLLOUT);
	int res = ::connect(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr));
	if(res == SOCKET_ERROR)
	{
		if(SocketUtils::GetLastError() != EINPROGRESS)
		{
			return false;
		}
	}
#endif
	return true;
}

bool Session::RegisterDisconnect()
{
#ifdef _WIN32
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this(); // ADD_REF

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}
#else
	ProcessDisconnect();
#endif

	return true;
}

void Session::RegisterRecv()
{
	if (IsConnected() == false)
		return;

#ifdef _WIN32
	_recvEvent.Init();
	_recvEvent.owner = shared_from_this(); // ADD_REF

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT &numOfBytes, OUT &flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr; // RELEASE_REF
		}
	}
#endif
}

#ifndef _WIN32
void Session::FlushSend()
{
	int32 totalSentBytes = 0;
	while(true)
	{
		constexpr int MAX_IOV = 64;
		iovec iovs[MAX_IOV];
		int32 iovCount = 0;
		size_t totalBytes = 0;

		vector<SendBufferRef> pendingBuffers;
		{
			WRITE_LOCK;
			if(_sendQueue.empty())
			{
				_sendRegistered.store(false);
				break;
			}

			while(_sendQueue.empty() == false && pendingBuffers.size() < MAX_IOV)
			{
				auto& buffer = _sendQueue.front();
				int32 offset = (iovCount == 0) ? _sendOffset : 0;

				iovs[iovCount].iov_base = buffer->Buffer() + offset;
				iovs[iovCount].iov_len = buffer->WriteSize() - offset;

				totalBytes += iovs[iovCount].iov_len;
				pendingBuffers.push_back(buffer);
				++iovCount;
			}
		}

		ssize_t sentBytes = ::writev(_socket, iovs, iovCount);
		totalSentBytes += static_cast<int32>(sentBytes);
		if(sentBytes == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK) break;
			Disconnect("Writev Error");
			return;
		}

		{
			WRITE_LOCK;
			auto bytesToRemove = static_cast<int32>(sentBytes);

			while (bytesToRemove > 0 && _sendQueue.empty() == false)
			{
				const auto& buffer = _sendQueue.front();
				int32 dataInBuf = buffer->WriteSize() - _sendOffset;
				if(bytesToRemove >= dataInBuf)
				{
					bytesToRemove -= dataInBuf;
					_sendQueue.pop();
					_sendOffset = 0;
				}
				else
				{
					_sendOffset += bytesToRemove;
					bytesToRemove = 0;
				}
			}

			if(_sendQueue.empty())
			{
				_sendRegistered.store(false);
				break;
			}
		}
		if(sentBytes < totalBytes) break;
	}
	if(totalSentBytes > 0)
		OnSend(totalSentBytes);
}
#endif


void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;
#ifdef _WIN32
	_sendEvent.Init();
	_sendEvent.owner = shared_from_this(); // ADD_REF

	// 보낼 데이터를 sendEvent에 등록
	{
		int32 writeSize = 0;
		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();

			writeSize += sendBuffer->WriteSize();
			// TODO : 예외 체크

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// Scatter-Gather (흩어져 있는 데이터들을 모아서 한 방에 보낸다)
	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT &numOfBytes, 0, &_sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.owner = nullptr; // RELEASE_REF
			_sendEvent.sendBuffers.clear(); // RELEASE_REF
			_sendRegistered.store(false);
		}
	}
#else
	FlushSend();
	if(_sendRegistered.load())
		GetService()->GetNetCore()->Update(shared_from_this(), EPOLLIN | EPOLLOUT);
#endif
}

void Session::ProcessConnect()
{
	// 세션 등록
	GetService()->AddSession(GetSessionRef());

#ifdef _WIN32
	_connectEvent.owner = nullptr; // RELEASE_REF
#endif

	_connected.store(true);

	// 컨텐츠 코드에서 재정의
	OnConnected();

	// 수신 등록
	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	OnDisconnected(); // 컨텐츠 코드에서 재정의

	GetService()->GetNetCore()->UnRegister(shared_from_this());
	SocketUtils::Close(_socket);

	GetService()->ReleaseSession(GetSessionRef());
#ifdef _WIN32
	_disconnectEvent.owner = nullptr; // RELEASE_REF
#else
	_epollRef.reset();
#endif
}

void Session::ProcessRecv(int32 numOfBytes)
{
#ifdef _WIN32
	_recvEvent.owner = nullptr; // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect("Recv 0");
		return;
	}

	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect("OnWrite Overflow");
		return;
	}
#else
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
#endif

	int32 dataSize = _recvBuffer.DataSize();
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize); // 컨텐츠 코드에서 재정의
	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect("OnRead Overflow");
		return;
	}
	
	// 커서 정리
	_recvBuffer.Clean();

	// 수신 등록
	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
#ifdef _WIN32
	_sendEvent.owner = nullptr; // RELEASE_REF
	_sendEvent.sendBuffers.clear(); // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect("Send 0");
		return;
	}

	// 컨텐츠 코드에서 재정의
	OnSend(numOfBytes);

	WRITE_LOCK;
	if (_sendQueue.empty())
		_sendRegistered.store(false);
	else
		RegisterSend();
#endif

}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
#ifdef _WIN32
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect("HandleError");
		break;
#endif
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

/*-----------------
	PacketSession
------------------*/

// [size(2)][id(2)][data....][size(2)][id(2)][data....]
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다
		if (dataSize < header.size)
			break;

		// 패킷 조립 성공
		OnRecvPacket(&buffer[processLen], header.size);

		processLen += header.size;
	}

	return processLen;
}
