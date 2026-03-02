#include "pch.h"
#include "NetCore.h"
#include "NetObject.h"
#include "NetEvent.h"

NetCore::NetCore()
{
	_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (_handle == INVALID_HANDLE_VALUE)
	{
		// TODO : Log
	}
}

NetCore::~NetCore()
{
	if (_handle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(_handle);
	}
}

bool NetCore::Register(const NetObjectRef& netObject)
{
	return ::CreateIoCompletionPort(netObject->GetHandle(), _handle, 0, 0) != nullptr;
}

void NetCore::UnRegister(const NetObjectRef& netObject)
{

}

bool NetCore::Dispatch(int32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	NetEvent* netEvent = nullptr;

	if (::GetQueuedCompletionStatus(_handle, &numOfBytes, &key, reinterpret_cast<LPOVERLAPPED*>(&netEvent), timeoutMs))
	{
		netEvent->owner->Dispatch(netEvent, numOfBytes);
	}
	else
	{
		switch (::WSAGetLastError())
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			// TODO : Log
			if (netEvent)
				netEvent->owner->Dispatch(netEvent, numOfBytes);
			break;
		}
	}

	return true;
}

bool NetCore::Update(const NetObjectRef& netObject, uint32 eventFlags)
{
	return true;
}
