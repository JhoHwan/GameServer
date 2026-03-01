#pragma once

enum class EventType : uint8
{
    Connect,
    Disconnect,
    Accept,
    //PreRecv,
    Recv,
    Send
};

#ifdef _WIN32
    #include "IocpEvent.h"
    using NetEvent = IocpEvent;
#else
    #include "EpollEvent.h"
    using NetEvent = EpollEvent;
#endif


/*----------------
    ConnectEvent
-----------------*/

class ConnectEvent : public NetEvent
{
public:
    ConnectEvent() : NetEvent(EventType::Connect) { }
};

/*--------------------
    DisconnectEvent
----------------------*/

class DisconnectEvent : public NetEvent
{
public:
    DisconnectEvent() : NetEvent(EventType::Disconnect) { }
};

/*----------------
    AcceptEvent
-----------------*/

class AcceptEvent : public NetEvent
{
public:
    AcceptEvent() : NetEvent(EventType::Accept) { }

public:
    SessionRef	session = nullptr;
};

/*----------------
    RecvEvent
-----------------*/

class RecvEvent : public NetEvent
{
public:
    RecvEvent() : NetEvent(EventType::Recv) { }
};

/*----------------
    SendEvent
-----------------*/

class SendEvent : public NetEvent
{
public:
    SendEvent() : NetEvent(EventType::Send) { }

    vector<SendBufferRef> sendBuffers;
};