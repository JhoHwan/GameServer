#include "pch.h"
#include "SocketUtil.h"
#include "NetAddress.h"

LPFN_DISCONNECTEX SocketUtil::DisconnectEx = nullptr;
LPFN_CONNECTEX SocketUtil::ConnectEx = nullptr;

SOCKET SocketUtil::CreateSocket() 
{
    // Overlapped I/O를 지원하는 소켓 생성
    return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
}

bool SocketUtil::Bind(SOCKET socket, const NetAddress& address) 
{
    // NetAddress 객체에서 sockaddr_in 구조체 가져오기
    const sockaddr_in& sockAddr = address.GetAddress();

    // 소켓을 특정 IP와 포트에 바인딩
    return ::bind(socket, reinterpret_cast<const sockaddr*>(&sockAddr), sizeof(sockAddr)) != SOCKET_ERROR;
}

bool SocketUtil::BindAnyAddress(SOCKET socket, uint16 port)
{
    if (socket == INVALID_SOCKET)
    {
        return false;
    }

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    addr.sin_port = ::htons(port);

    return SOCKET_ERROR != ::bind(socket, reinterpret_cast<const SOCKADDR*>(&addr), sizeof(addr));
}

bool SocketUtil::Listen(SOCKET socket, int backlog) 
{
    // 소켓을 클라이언트 연결 요청 수신 대기 상태로 설정
    return ::listen(socket, backlog) != SOCKET_ERROR;
}

bool SocketUtil::SetReuseAddr(SOCKET socket) 
{
    int optval = 1; // 옵션 활성화 값

    // setsockopt을 사용하여 SO_REUSEADDR 옵션 설정 (주소 재사용 허용)
    return ::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval)) == 0;
}

bool SocketUtil::SetNoDelay(SOCKET socket) 
{
    int optval = 1; // 옵션 활성화 값

    // setsockopt을 사용하여 TCP_NODELAY 설정 (Nagle 알고리즘 비활성화)
    return ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&optval), sizeof(optval)) == 0;
}

bool SocketUtil::SetAcceptSockOption(SOCKET acceptedSocket, SOCKET listenSocket) 
{
    // Accept된 소켓이 listen 소켓의 속성을 상속받도록 SO_UPDATE_ACCEPT_CONTEXT 설정
    return ::setsockopt(acceptedSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<const char*>(&listenSocket), sizeof(listenSocket)) == 0;
}

bool SocketUtil::GetNetAddressBySocket(SOCKET socket, NetAddress& netAddress)
{
    SOCKADDR_IN sockAddress;
    int32 sizoOfSockAddr = sizeof(sockAddress);
    if (SOCKET_ERROR == ::getpeername(socket, OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizoOfSockAddr))
    {
        return false;
    }

    netAddress = NetAddress(sockAddress);
    return true;
}

bool SocketUtil::SetLinger(SOCKET socket, uint16 onoff, uint16 linger)
{
    LINGER option;
    option.l_onoff = onoff;
    option.l_linger = linger;
    return SOCKET_ERROR != setsockopt(socket, SOL_SOCKET, SO_LINGER, (char*) & option, sizeof(option));
}

void SocketUtil::CloseSocket(SOCKET socket) {
    // 소켓이 유효하면 닫기
    if (socket != INVALID_SOCKET) ::closesocket(socket);
    socket = INVALID_SOCKET;
}

LPFN_DISCONNECTEX SocketUtil::GetDisconnectEx()
{
    if (DisconnectEx == nullptr) LoadDisconnectEx();
    return DisconnectEx;
}

LPFN_CONNECTEX SocketUtil::GetConnectEx()
{
    if (ConnectEx == nullptr) LoadConnectEx();
    return ConnectEx;
}

void SocketUtil::LoadDisconnectEx()
{
    SOCKET socket = SocketUtil::CreateSocket();
    DWORD bytesReturned = 0;
    GUID guidDisconnectEx = WSAID_DISCONNECTEX;
    if (SOCKET_ERROR == ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidDisconnectEx, sizeof(guidDisconnectEx), &DisconnectEx, sizeof(DisconnectEx), &bytesReturned, nullptr, nullptr))
    {
        cout << "LoadDisconnectEx Error" << WSAGetLastError() << endl;
    }

    SocketUtil::CloseSocket(socket);
}

void SocketUtil::LoadConnectEx()
{
    SOCKET socket = SocketUtil::CreateSocket();
    DWORD bytesReturned = 0;
    GUID guidConnectEx = WSAID_CONNECTEX;
    if (SOCKET_ERROR == ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx), &ConnectEx, sizeof(ConnectEx), &bytesReturned, nullptr, nullptr))
    {
        cout << "LoadConnectEx Error" << WSAGetLastError() << endl;
    }

    SocketUtil::CloseSocket(socket);
}

