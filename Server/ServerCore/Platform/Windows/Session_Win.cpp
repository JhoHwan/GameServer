#include "Session_Win.h"

#include "Service.h"
#include "SocketUtils.h"

SessionImpl_Win::SessionImpl_Win(Session *owner) : _owner(owner)
{
    _socket = SocketUtils::CreateSocket();
}

SessionImpl_Win::~SessionImpl_Win()
{
    SocketUtils::Close(_socket);
}

void SessionImpl_Win::Dispatch(NetEvent* netEvent, int32 numOfBytes)
{
    switch (netEvent->eventType)
    {
        case EventType::Connect:
            ProcessConnect();
            break;
        case EventType::Disconnect:
            ProcessDisconnect();
            break;
        case EventType::Recv:
            ProcessRecv(numOfBytes);
            break;
        case EventType::Send:
            ProcessSend(numOfBytes);
            break;
        default:
            break;
    }
}

void SessionImpl_Win::Send()
{
    RegisterSend();
}

void SessionImpl_Win::RegisterSend()
{
    if (_owner->IsConnected() == false)
        return;

    _sendEvent.Init();
    _sendEvent.owner = _owner->shared_from_this(); // ADD_REF

    {
        // 보낼 데이터를 sendEvent에 등록
        int32 writeSize = 0;
        SendBufferRef sendBuffer;
        while (_owner->_sendQueue.try_dequeue(sendBuffer))
        {
            writeSize += sendBuffer->WriteSize();

            _sendEvent.sendBuffers.push_back(sendBuffer);
        }
    }

    // Scatter-Gather (흩어져 있는 데이터들을 모아서 한 방에 보낸다)
    vector<WSABUF> wsaBufs;
    wsaBufs.reserve(_sendEvent.sendBuffers.size());
    for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
    {
        WSABUF wsaBuf;
        wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
        wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
        wsaBufs.push_back(wsaBuf);
    }

    DWORD numOfBytes = 0;
    if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT &numOfBytes, 0, &_sendEvent, nullptr))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            HandleError(errorCode);
            _sendEvent.owner = nullptr; // RELEASE_REF
            _sendEvent.sendBuffers.clear(); // RELEASE_REF
            _owner->_sendRegistered.store(false);
        }
    }
}


void SessionImpl_Win::ProcessSend(int32 numOfBytes)
{
    _sendEvent.owner = nullptr; // RELEASE_REF
    _sendEvent.sendBuffers.clear(); // RELEASE_REF

    if (numOfBytes == 0)
    {
        _owner->Disconnect("Send 0");
        return;
    }

    // 컨텐츠 코드에서 재정의
    _owner->OnSend(numOfBytes);

    if (_owner->_sendQueue.size_approx() == 0)
        _owner->_sendRegistered.store(false);
    else
        RegisterSend();
}

void SessionImpl_Win::RegisterRecv()
{
    if (_owner->IsConnected() == false)
        return;

    _recvEvent.Init();
    _recvEvent.owner = _owner->shared_from_this(); // ADD_REF

    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<char*>(_owner->_recvBuffer.WritePos());
    wsaBuf.len = _owner->_recvBuffer.FreeSize();

    DWORD numOfBytes = 0;
    DWORD flags = 0;
    if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT &numOfBytes, OUT &flags, &_recvEvent, nullptr))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            HandleError(errorCode);
            _recvEvent.owner = nullptr; // RELEASE_REF
        }
    }
}

void SessionImpl_Win::ProcessRecv(int32 numOfBytes)
{
    _recvEvent.owner = nullptr; // RELEASE_REF

    if (numOfBytes == 0)
    {
        _owner->Disconnect("Recv 0");
        return;
    }

    if (_owner->_recvBuffer.OnWrite(numOfBytes) == false)
    {
        _owner->Disconnect("OnWrite Overflow");
        return;
    }

    int32 dataSize = _owner->_recvBuffer.DataSize();
    int32 processLen = _owner->OnRecv(_owner->_recvBuffer.ReadPos(), dataSize); // 컨텐츠 코드에서 재정의
    if (processLen < 0 || dataSize < processLen || _owner->_recvBuffer.OnRead(processLen) == false)
    {
        _owner->Disconnect("OnRead Overflow");
        return;
    }

    // 커서 정리
    _owner->_recvBuffer.Clean();

    // 수신 등록
    RegisterRecv();
}

void SessionImpl_Win::HandleError(int32 errorCode)
{
    switch (errorCode)
    {
        case WSAECONNRESET:
        case WSAECONNABORTED:
            _owner->Disconnect("HandleError");
            break;
        default:
            // TODO : Log
            cout << "Handle Error : " << errorCode << endl;
            break;
    }
}

bool SessionImpl_Win::Connect()
{
    return RegisterConnect();
}

void SessionImpl_Win::ProcessConnect()
{
    _connectEvent.owner = nullptr;
    RegisterRecv();
}

bool SessionImpl_Win::RegisterConnect()
{
    if (_owner->IsConnected())
        return false;

    if (_owner->GetService()->GetServiceType() != ServiceType::Client)
        return false;

    if (SocketUtils::SetReuseAddress(_socket, true) == false)
        return false;

    if (SocketUtils::BindAnyAddress(_socket, 0/*남는거*/) == false)
        return false;

    _connectEvent.Init();
    _connectEvent.owner = _owner->shared_from_this(); // ADD_REF

    DWORD numOfBytes = 0;
    SOCKADDR_IN sockAddr = _owner->GetService()->GetNetAddress().GetSockAddr();
    if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            _connectEvent.owner = nullptr; // RELEASE_REF
            return false;
        }
    }

    return true;
}

void SessionImpl_Win::Disconnect()
{
    RegisterDisconnect();
}

bool SessionImpl_Win::RegisterDisconnect()
{
    _disconnectEvent.Init();
    _disconnectEvent.owner = _owner->shared_from_this(); // ADD_REF

    if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            _disconnectEvent.owner = nullptr; // RELEASE_REF
            return false;
        }
    }

    return true;
}

void SessionImpl_Win::ProcessDisconnect()
{
    _disconnectEvent.owner = nullptr; // RELEASE_REF

    _owner->OnDisconnected(); // 컨텐츠 코드에서 재정의
    _owner->GetService()->ReleaseSession(_owner->GetSessionRef());
}
