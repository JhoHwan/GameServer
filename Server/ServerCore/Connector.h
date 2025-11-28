#pragma once
class Connector : public IOCPObject
{
public:
	Connector();
	~Connector();

	void Connect(wstring ip, uint16 port, shared_ptr<Session> session);

	void Dispatch(IOCPEvent* iocpEvent, int32 numOfBytes) override;

private:
	bool RegisterConnect(wstring ip, uint16 port, ConnectEvent* connectEvent);


private:

};
