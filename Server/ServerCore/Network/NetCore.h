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
#ifndef _WIN32
	struct SessionSlot
	{
		atomic<NetObjectRef> netObject = nullptr;
		atomic<uint32> gen {0};
	};
	SessionSlot _registry[65536]{};
#endif
};
