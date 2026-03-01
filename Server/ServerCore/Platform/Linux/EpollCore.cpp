#include "EpollCore.h"

EpollCore::EpollCore()
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        perror("epoll_create1 error");
    }
}

EpollCore::~EpollCore()
{
    close(_epoll_fd);
}

bool EpollCore::Register(NetObjectRef netObject)
{
    const SOCKET socket =  netObject->GetHandle();
    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = netObject.get();
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket, &event) == -1)
    {
        return false;
    }

    netObject->_epollRef = netObject;
    return true;
}

bool EpollCore::Dispatch(uint32 timeoutMs)
{
    constexpr int MAX_EVENTS = 16;
    epoll_event events[MAX_EVENTS];

    int numEvents = epoll_wait(_epoll_fd, events, MAX_EVENTS, timeoutMs);
    if (numEvents == -1)
    {
        if (errno == EINTR)
            return true;
        perror("epoll_wait error");
        return false;
    }

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

