#pragma once

class NetCore
{
public:
    virtual ~NetCore() = default;

    virtual HANDLE GetHandle() const = 0;
    virtual bool Register(NetObjectRef netObject) = 0;
    virtual bool Dispatch(uint32 timeoutMs) = 0;
};