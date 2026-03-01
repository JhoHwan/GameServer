#pragma once

class NetObject : public std::enable_shared_from_this<NetObject>
{
public:
    virtual ~NetObject() = default;
    virtual HANDLE GetHandle() = 0;
    virtual void Dispatch(NetEvent* netEvent, int32 numOfBytes) = 0;

#ifndef _WIN32
protected:
    friend class EpollCore;
    NetObjectRef _epollRef;
#endif
};
