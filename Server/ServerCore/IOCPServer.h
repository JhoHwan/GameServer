#pragma once
#include "NetAddress.h"
#include "Listener.h"
#include "Session.h"

class PacketHandler;
class IOCPCore;

using SessionFactory = function<shared_ptr<Session>(void)>;

class IOCPServer : public std::enable_shared_from_this<IOCPServer>
{
public:
    // 생성자 및 소멸자
    IOCPServer() = delete;
    IOCPServer(NetAddress address, SessionFactory sessionFactory);
    ~IOCPServer();

    shared_ptr<Session> CreateSession();

    virtual void BroadCast(shared_ptr<SendBuffer> sendBuffer);

    // 서버 관리
    bool Start();
    bool Stop();

    // 세션 관리
    void AddSession(std::shared_ptr<Session> session);
    void DeleteSession(std::shared_ptr<Session> session);

    // 이벤트 디스패처
    void DispatchIocpEvent(uint16);
    void Dispatch(uint16 iocpDispatchTime);

    // Getter
    inline const NetAddress& GetAddress() const { return _address; }
    inline IOCPCore& GetIOCPCore() { return _iocpCore; }

protected:
	bool _isRunning = false;

    IOCPCore _iocpCore;

    NetAddress _address;

    shared_ptr<Listener> _listener;

    set<std::shared_ptr<Session>> _sessions;
    SessionFactory _sessionFactory;

	vector<std::thread> _iocpThreads;
// Mutex
protected:
    mutex _lock;
};
