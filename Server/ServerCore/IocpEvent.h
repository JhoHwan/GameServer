#pragma once
//#include "Session.h"

class Session;
class SendBuffer;
class IOCPObject;

enum class EventType : uint8
{
	Accept,
	Connect,
	RegisterSend,
	Send, 
	Recv,
	Disconnect,
};

class IOCPEvent
{
public:
	IOCPEvent(EventType eventType) : _eventType(eventType) { Init(); }
	inline void Init() {_overlapped = {}; }

	inline EventType GetEventType() const { return _eventType; }

private:
	OVERLAPPED _overlapped{};
	EventType _eventType;

public:
	shared_ptr<IOCPObject> owner;
};

class AcceptEvent : public IOCPEvent
{
public:
	AcceptEvent() : IOCPEvent(EventType::Accept) {}

	shared_ptr<Session> session;
};

class ConnectEvent : public IOCPEvent
{
public:
	ConnectEvent() : IOCPEvent(EventType::Connect) {}

	shared_ptr<Session> session;
};

class RegisterSendEvent : public IOCPEvent
{
public:
	RegisterSendEvent() : IOCPEvent(EventType::RegisterSend) {}
};

class SendEvent : public IOCPEvent
{
public:
	SendEvent() : IOCPEvent(EventType::Send) {}

	vector <shared_ptr<SendBuffer>> sendBuffers;
};

class RecvEvent : public IOCPEvent
{
public:
	RecvEvent() : IOCPEvent(EventType::Recv) {}

};

class DisconnectEvent : public IOCPEvent
{
public:
	DisconnectEvent() : IOCPEvent(EventType::Disconnect) {}

};