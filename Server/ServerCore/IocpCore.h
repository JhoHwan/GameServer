#pragma once
class IOCPCore
{
public:
    IOCPCore();
    ~IOCPCore();

    // IOCP µî·Ï
    bool Register(HANDLE handle);
    bool RegisterSocket(SOCKET socket);

    bool Dispatch(uint32 time = INFINITE);

    HANDLE GetHandle() const { return _iocpHandle; }

private:
    HANDLE _iocpHandle;

};