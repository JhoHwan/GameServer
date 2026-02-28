#pragma once

class NetObject : public std::enable_shared_from_this<NetObject>
{
public:
    virtual ~NetObject() = default;

    virtual HANDLE GetHandle() = 0;
    virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes) = 0;
};
