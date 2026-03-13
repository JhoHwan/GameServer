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
    const HANDLE socket = netObject->GetHandle();
    auto gen = _registry[socket].gen.fetch_add(1)+1;
    _registry[socket].netObject.store(netObject);
    uint64 ticket =  (static_cast<uint64>(gen) << 32) | socket;

    epoll_event event{};
    event.events = netObject->_nativeFlags;
    event.data.u64 = ticket;

    if (epoll_ctl(_handle, EPOLL_CTL_ADD, socket, &event) == -1)
    {
        perror("epoll_ctl ADD error");
        return false;
    }

    return true;
}

void NetCore::UnRegister(const NetObjectRef& netObject)
{
    const HANDLE socket = netObject->GetHandle();
    _registry[socket].netObject.store(nullptr);

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
        uint64 ticket = events[i].data.u64;
        uint32 fd = static_cast<uint32>(ticket & 0xFFFFFFFF);
        uint32 gen = static_cast<uint32>(ticket >> 32);

        NetObjectRef netObject = nullptr;
        if(_registry[fd].gen.load() == gen)
        {
            netObject = _registry[fd].netObject.load();
        }

        if(!netObject) return false;

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

    uint32 gen = _registry[socket].gen.load();
    uint64 ticket = (static_cast<uint64>(gen) << 32) | socket;

    event.events = eventFlags | EPOLLET | EPOLLONESHOT;
    event.data.u64 = ticket;

    if (epoll_ctl(_handle, EPOLL_CTL_MOD, socket, &event) == -1)
    {
        perror("epoll_ctl MOD error");
        return false;
    }

    return true;
}
