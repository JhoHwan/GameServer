#pragma once

class NetCore
{
public:
	NetCore();
	~NetCore();

	HANDLE GetHandle() const { return _handle; }
	bool Register(const NetObjectRef& netObject);
	void UnRegister(const NetObjectRef& netObject);
	bool Dispatch(int32 timeoutMs = INFINITE);
	bool Update(const NetObjectRef& netObject, uint32 eventFlags);

private:
	HANDLE _handle = INVALID_HANDLE_VALUE;
};