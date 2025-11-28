#pragma once
class IOCPObject : public enable_shared_from_this<IOCPObject>
{
public:
	virtual void Dispatch(class IOCPEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};