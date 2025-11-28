#pragma once

class IOCPServer;
class AcceptEvent;

class Listener : public IOCPObject
{
public:
    // 积己磊 棺 家戈磊
    Listener(unsigned int maxAcceptNum = 100);
    ~Listener();

    // Accept 包府
    bool StartAccept(std::shared_ptr<IOCPServer> server);
    void RegisterAccept(AcceptEvent* acceptEvent);
    void ProcessAccept(AcceptEvent* acceptEvent);

    void Dispatch(IOCPEvent* iocpEvent, int32 numOfBytes) override;

    // Getter
    inline const SOCKET& GetSocket() const { return _socket; }

private:
    const unsigned int MAX_ACCEPT_NUM;
    std::shared_ptr<IOCPServer> _server;
    std::vector<AcceptEvent> _acceptEvents; 
    SOCKET _socket = {};


};