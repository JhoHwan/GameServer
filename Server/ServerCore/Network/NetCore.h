#pragma once

class NetCore
{
public:
    virtual ~NetCore() = default;

    virtual HANDLE GetHandle() = 0;
    virtual bool Register(NetObjectRef iocpObject) = 0;
    virtual bool Dispatch(uint32 timeoutMs) = 0;
};