#pragma once
#include "NetAddress.h"

class AcceptEvent;
class ServerService;

/*--------------
	Listener
---------------*/

class Listener : public NetObject
{
	friend class ListenerImpl;

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
	unique_ptr<class ListenerImpl> _impl;

	SOCKET _socket = INVALID_SOCKET;
	ServerServiceRef _service;
};

