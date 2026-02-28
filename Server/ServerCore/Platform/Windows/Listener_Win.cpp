#include "Listener_Win.h"

#include "Service.h"
#include "SocketUtils.h"
#include "Types.h"
#include "NetEvent.h"

unique_ptr<ListenerImpl> ListenerImpl::CreateListenerImpl(Listener *owner)
{
    return make_unique<ListenerImpl>(owner);
}

ListenerImpl::ListenerImpl(Listener *owner) : _owner(owner)
{
}

bool ListenerImpl::StartAccept(const ServerServiceRef& service)
{
    const int32 acceptCount = service->GetMaxSessionCount();
    for (int32 i = 0; i < acceptCount; i++)
    {
        auto* acceptEvent = new AcceptEvent();
        acceptEvent->owner = _owner->shared_from_this();
        RegisterAccept(acceptEvent);
    }

    return true;
}

void ListenerImpl::Dispatch(NetEvent *netEvent, int32 numOfBytes)
{
    ASSERT_CRASH(netEvent->eventType == EventType::Accept);
    auto acceptEvent = reinterpret_cast<AcceptEvent*>(netEvent);
    ProcessAccept(acceptEvent);
}

void ListenerImpl::RegisterAccept(AcceptEvent* acceptEvent)
{
    if (_owner->_socket == INVALID_SOCKET)
    {
        delete acceptEvent;
        return;
    }

    SessionRef session = _owner->_service->CreateSession(); // Register IOCP

    acceptEvent->Init();
    acceptEvent->session = session;

    DWORD bytesReceived = 0;
    if (SocketUtils::AcceptEx(
        _owner->_socket,
        session->GetSocket(),
        session->_recvBuffer.WritePos(),
        0,
        sizeof(SOCKADDR_IN) + 16,
        sizeof(SOCKADDR_IN) + 16,
        OUT & bytesReceived,
        static_cast<LPOVERLAPPED>(acceptEvent)) == false)
    {
        const int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            delete acceptEvent;
        }
    }
}

void ListenerImpl::ProcessAccept(AcceptEvent* acceptEvent)
{
    if (_owner->_socket == INVALID_SOCKET)
    {
        delete acceptEvent;
        return;
    }

    SessionRef session = acceptEvent->session;

    if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _owner->_socket))
    {
        RegisterAccept(acceptEvent);
        return;
    }

    SOCKADDR_IN sockAddress;
    int32 sizeOfSockAddr = sizeof(sockAddress);
    if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
    {
        RegisterAccept(acceptEvent);
        return;
    }

    session->SetNetAddress(NetAddress(sockAddress));
    session->ProcessConnect();
    RegisterAccept(acceptEvent);
}