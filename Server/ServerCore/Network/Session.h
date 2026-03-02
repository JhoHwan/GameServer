#pragma once
#include "NetEvent.h"
#include "NetAddress.h"
#include "NetObject.h"
#include "RecvBuffer.h"

class Service;

/*--------------
	Session
---------------*/

class Session : public NetObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	~Session() override;

public:
						/* 외부에서 사용 */
	void				Send(SendBufferRef sendBuffer);
	bool				Connect();
	void				Disconnect(const char* cause);

	shared_ptr<Service>	GetService() const { return _service.lock(); }
	void				SetService(const shared_ptr<Service>& service) { _service = service; }

public:
						/* 정보 관련 */
	void				SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress			GetAddress() const { return _netAddress; }
	void				SetSocket(const SOCKET& socket) { _socket = socket; }
	SOCKET				GetSocket() const { return _socket; }
	bool				IsConnected() { return _connected; }
	SessionRef			GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
						/* 인터페이스 구현 */
	HANDLE		GetHandle() override;
	void		Dispatch(NetEvent* netEvent, int32 numOfBytes) override;

private:
						/* 전송 관련 */
	bool				RegisterConnect();
	bool				RegisterDisconnect();
	void				RegisterRecv();

#ifndef _WIN32
	void FlushSend();
#endif

	void				RegisterSend();

	void				ProcessConnect();
	void				ProcessDisconnect();
	void				ProcessRecv(int32 numOfBytes);
	void				ProcessSend(int32 numOfBytes);

	void				HandleError(int32 errorCode);

protected:
						/* 컨텐츠 코드에서 재정의 */
	virtual void		OnConnected() { }
	virtual int32		OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void		OnSend(int32 len) { }
	virtual void		OnDisconnected() { }

private:
	weak_ptr<Service>	_service;
	SOCKET				_socket = INVALID_SOCKET;
	NetAddress			_netAddress = {};
	atomic<bool>		_connected = false;

private:
	USE_LOCK;
							/* 수신 관련 */
	RecvBuffer				_recvBuffer;

							/* 송신 관련 */
	queue<SendBufferRef>	_sendQueue;
	atomic<bool>			_sendRegistered = false;

#ifdef _WIN32
						/* IocpEvent 재사용 */
	ConnectEvent		_connectEvent{};
	DisconnectEvent		_disconnectEvent{};
	RecvEvent			_recvEvent{};
	SendEvent			_sendEvent{};
#else
	int32 _sendOffset = 0;
#endif
};

/*-----------------
	PacketSession
------------------*/

struct PacketHeader
{
	uint16 size;
	uint16 id; // 프로토콜ID (ex. 1=로그인, 2=이동요청)
};

class PacketSession : public Session
{
public:
	PacketSession() = default;
	~PacketSession() override = default;

	PacketSessionRef GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	int32 OnRecv(BYTE* buffer, int32 len) final;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) = 0;
};