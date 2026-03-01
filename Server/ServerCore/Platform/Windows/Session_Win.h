#pragma once
#include <memory>

#include "CorePch.h"

class SessionImpl_Win
{
public:
    static unique_ptr<SessionImpl_Win> CreateSessionImpl(Session* owner)
    {
        return std::make_unique<SessionImpl_Win>(owner);
    }

    SessionImpl_Win(Session* owner);
    ~SessionImpl_Win();

public:
    HANDLE GetHandle() const { return reinterpret_cast<HANDLE>(_socket); }

    void Dispatch(NetEvent* netEvent, int32 numOfBytes = 0);
    void Send();
    bool Connect();
    void ProcessConnect();
    void Disconnect();

    void HandleError(int32 errorCode);

private:

    bool				RegisterConnect();
    bool				RegisterDisconnect();
    void				RegisterRecv();
    void				RegisterSend();

    void				ProcessDisconnect();
    void				ProcessRecv(int32 numOfBytes);
    void				ProcessSend(int32 numOfBytes);


private:
    USE_LOCK;

    Session* _owner = nullptr;
	SOCKET				_socket = INVALID_SOCKET;

    ConnectEvent		_connectEvent;
    DisconnectEvent		_disconnectEvent;
    RecvEvent			_recvEvent;
    SendEvent			_sendEvent;
};
