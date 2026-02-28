#pragma once
#include "NetCore.h"
#include "NetObject.h"
/*--------------
	IocpCore
---------------*/

class IocpCore : public NetCore
{
public:
	IocpCore();
	~IocpCore() override;

	HANDLE GetHandle() override { return _iocpHandle; }

	bool Register(NetObjectRef iocpObject) override;
	bool Dispatch(uint32 timeoutMs) override;

private:
	HANDLE		_iocpHandle;
};