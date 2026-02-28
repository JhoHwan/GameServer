#pragma once

class Session;

enum class EventType : uint8;

/*--------------
	IocpEvent
---------------*/

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);

	void			Init();

public:
	EventType		eventType;
	NetObjectRef	owner;
};