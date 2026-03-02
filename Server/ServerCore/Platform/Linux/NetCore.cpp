#include "pch.h"
#include "NetCore.h"
#include "NetObject.h"
#include "NetEvent.h"

NetCore::NetCore()
{
    _handle = epoll_create1(0);
    if (_handle == -1)
    {
        perror("epoll_create1 error");
    }
}

NetCore::~NetCore()
{
    if (_handle != -1)
    {
        close(_handle);
    }
}

bool NetCore::Register(const NetObjectRef& netObject)
{
    const auto socket = netObject->GetHandle();
    epoll_event event{};
    event.events = netObject->_nativeFlags;
    event.data.ptr = netObject.get();

    if (epoll_ctl(_handle, EPOLL_CTL_ADD, socket, &event) == -1)
    {
        perror("epoll_ctl ADD error");
        return false;
    }

    netObject->_epollRef = netObject;
    return true;
}

void NetCore::UnRegister(const NetObjectRef& netObject)
{
    const auto socket = netObject->GetHandle();
    if(socket != INVALID_SOCKET)
        epoll_ctl(_handle, EPOLL_CTL_DEL, socket, nullptr);
}

bool NetCore::Dispatch(const int32 timeoutMs)
{
    constexpr int MAX_EVENTS = 128;
    epoll_event events[MAX_EVENTS];

    int numEvents = epoll_wait(_handle, events, MAX_EVENTS, timeoutMs);
    if (numEvents == -1)
    {
        if (errno == EINTR) return true;
        perror("epoll_wait error");
        return false;
    }

    // Time Out
    if (numEvents == 0)
        return false;

    for (int i = 0; i < numEvents; ++i)
    {
        auto* netObject = static_cast<NetObject*>(events[i].data.ptr);

        NetEvent netEvent{};
        netEvent.eventFlags = events[i].events;

        netObject->Dispatch(&netEvent, 0);
    }

    return true;
}

bool NetCore::Update(const NetObjectRef& netObject, uint32 eventFlags)
{
    const auto socket = netObject->GetHandle();
    if(socket == INVALID_SOCKET) return false;
    epoll_event event{};

    event.events = eventFlags | EPOLLET | EPOLLONESHOT;
    event.data.ptr = netObject.get();

    if (epoll_ctl(_handle, EPOLL_CTL_MOD, socket, &event) == -1)
    {
        perror("epoll_ctl MOD error");
        return false;
    }

    return true;
}
