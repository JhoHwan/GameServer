#pragma once

#include "NetObject.h"

class AcceptEvent;
class ServerService;
class NetEvent;

/*--------------
	Listener
---------------*/

class Listener : public NetObject
{
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

private:
	/* 수신 관련 */
	void RegisterAccept(AcceptEvent* acceptEvent);
	void ProcessAccept(NetEvent* netEvent);

protected:
	SOCKET _socket = INVALID_SOCKET;
	vector<AcceptEvent*> _acceptEvents;
	ServerServiceRef _service;
};

