#pragma once

#include "NetAddress.h"
#include "IOCPEvent.h"
#include "RecvBuffer.h"

class IOCPServer;
class SendBuffer;
class PacketHandler;

class Session : public IOCPObject
{
    enum { MAX_SEND_SIZE = 4096 };
public:
    // 생성자 및 소멸자
    Session();
    virtual ~Session();

    void CreateSocket();

    virtual void Dispatch(class IOCPEvent* iocpEvent, int32 numOfBytes = 0) override;

    // 연결 관리
    void Connect();
    void ProcessConnect();

    void Disconnect();
    void RegisterDisconnect();
    void ProcessDisconnect();

    // 데이터 전송 관리
    void Send(shared_ptr<SendBuffer> sendBuffer);
    void RegisterSend();
    void ProcessSend(uint32 sentBytes);

    // 데이터 수신 관리
    void RegisterRecv();
    void ProcessRecv(uint32 recvBytes);

    // 이벤트 핸들러
    virtual void OnConnected();
    virtual void OnDisconnected();
    virtual void OnSend(uint32 sentBytes);
    virtual void OnRecv(BYTE* buffer, int32 len);

    // Setter
    inline void SetServer(std::shared_ptr<IOCPServer> server) { _server = server; }
    inline void SetIOCPHandle(HANDLE iocpHandle) { _iocpHandle = iocpHandle; }
    inline void SetAddress(const NetAddress& address) { _address = address; }

    // Getteter 
    inline SOCKET GetSocket() { return _socket; }
    inline RecvBuffer& GetRecvBuffer() { return _recvBuffer; }
    inline const NetAddress& GetAddress() const { return _address; }
    inline shared_ptr<Session> GetSharedPtr() { return static_pointer_cast<Session>(shared_from_this()); }
    inline shared_ptr<IOCPServer> GetServer() { return _server; }


protected:
    atomic<bool> _isConnect;
    SOCKET _socket;
    std::shared_ptr<IOCPServer> _server;
    HANDLE _iocpHandle;

    NetAddress _address;

    RecvBuffer _recvBuffer;
    ConcurrentQueue<shared_ptr<SendBuffer>> _sendQueue;

    atomic<bool> _sendRegistered;
//IOCP Event
protected:
    RecvEvent _recvEvent;
    RegisterSendEvent _registerSendEvent;
    SendEvent _sendEvent;
    DisconnectEvent _disConnectEvent;
};

