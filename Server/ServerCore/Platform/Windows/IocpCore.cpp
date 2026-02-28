#include "IocpCore.h"
#include "IocpEvent.h"
#include "Types.h"

/*--------------
	IocpCore
---------------*/

IocpCore::IocpCore()
{
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(NetObjectRef netObjectRef)
{
	return ::CreateIoCompletionPort(netObjectRef->GetHandle(), _iocpHandle, /*key*/0, 0);
}

bool IocpCore::Dispatch(const uint32 timeoutMs)
{
	constexpr int MAX_ENTRIES = 16;
	OVERLAPPED_ENTRY entryList[MAX_ENTRIES];

	ULONG numEntriesRemoved = 0;

	bool success = GetQueuedCompletionStatusEx(
		_iocpHandle,
		entryList,
		MAX_ENTRIES,
		&numEntriesRemoved,
		timeoutMs,
		false);

	if (success)
	{
		for (ULONG i = 0; i < numEntriesRemoved; i++)
		{
			auto& entry = entryList[i];
			DWORD numOfBytes = entry.dwNumberOfBytesTransferred;
			ULONG_PTR key = entry.lpCompletionKey;

			IocpEvent* iocpEvent = static_cast<IocpEvent*>(entry.lpOverlapped);
			NetObjectRef iocpObject = iocpEvent->owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
		}
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			// TODO : 로그 찍기
			break;
		}
	}
	/*
	//if (::GetQueuedCompletionStatus(_iocpHandle, OUT &numOfBytes, OUT &key, OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	//{
	//	IocpObjectRef iocpObject = iocpEvent->owner;
	//	iocpObject->Dispatch(iocpEvent, numOfBytes);
	//}
	//else
	//{
	//	int32 errCode = ::WSAGetLastError();
	//	switch (errCode)
	//	{
	//	case WAIT_TIMEOUT:
	//		return false;
	//	default:
	//		IocpObjectRef iocpObject = iocpEvent->owner;
	//		iocpObject->Dispatch(iocpEvent, numOfBytes);
	//		break;
	//	}
	//}
	*/

	return true;
}
