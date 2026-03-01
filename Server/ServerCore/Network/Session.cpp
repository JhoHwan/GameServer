#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"
#include "Session_Win.h"

/*--------------
	Session
---------------*/

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	_impl = SessionImpl_Win::CreateSessionImpl(this);
}

Session::~Session()
{

}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;

	_sendQueue.enqueue(sendBuffer);

	if (_sendRegistered.exchange(true) == false)
		registerSend = true;

	if (registerSend)
		_impl->Send();
}

bool Session::Connect()
{
	return _impl->Connect();
}

void Session::Disconnect(const char* cause)
{
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	//cout << "Disconnect : " << cause << endl;

	_impl->Disconnect();
}

HANDLE Session::GetHandle()
{
	return _impl->GetHandle();
}

void Session::Dispatch(NetEvent* netEvent, int32 numOfBytes)
{
	_impl->Dispatch(netEvent, numOfBytes);
}

void Session::ProcessConnect()
{
	_connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 재정의
	OnConnected();

	_impl->ProcessConnect();
}

/*-----------------
	PacketSession
------------------*/

PacketSession::PacketSession()
{
}

PacketSession::~PacketSession()
{
}

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
