#pragma once

class NetCore;
class NetEvent;

class NetObject : public std::enable_shared_from_this<NetObject>
{
public:
    virtual ~NetObject() = default;
    virtual HANDLE GetHandle() = 0;
    virtual void Dispatch(NetEvent* netEvent, int32 numOfBytes) = 0;

protected:
#ifndef _WIN32
    friend class NetCore;
    uint32 _nativeFlags = 0;

    NetObjectRef _epollRef;
#endif
};
