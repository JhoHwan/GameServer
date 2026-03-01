#pragma once
#include "NetAddress.h"

class AcceptEvent;
class ServerService;

class ListenerImpl_Win;
class ListenerImpl_Linux;

/*--------------
	Listener
---------------*/

class Listener : public NetObject
{

#ifdef _WIN32
	using ListenerImpl = ListenerImpl_Win;
#else
	using ListenerImpl = ListenerImpl_Linux;
#endif

	friend ListenerImpl;

public:
	Listener();
	~Listener() override;

public:
	/* 외부에서 사용 */
	bool StartAccept(ServerServiceRef service);
	void CloseSocket();

public:
	/* 인터페이스 구현 */
	HANDLE GetHandle() override;
	void Dispatch(NetEvent* netEvent, int32 numOfBytes) override;

protected:
	unique_ptr<ListenerImpl> _impl;

	SOCKET _socket = INVALID_SOCKET;
	ServerServiceRef _service;
};


