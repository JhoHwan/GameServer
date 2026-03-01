#pragma once
#include "NetCore.h"


class EpollCore : public NetCore
{
public:
    EpollCore();
    ~EpollCore() override;

    HANDLE GetHandle() const override { return  _epoll_fd; }
    bool Register(NetObjectRef netObject) override;
    bool Dispatch(uint32 timeoutMs) override;

private:
    HANDLE _epoll_fd;
};
