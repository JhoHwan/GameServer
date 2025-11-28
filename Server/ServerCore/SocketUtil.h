#pragma once

class NetAddress;

class SocketUtil
{
public:
    // 소켓 생성
    static SOCKET CreateSocket();

    // 소켓 바인딩
    static bool Bind(SOCKET socket, const NetAddress& address);

    static bool BindAnyAddress(SOCKET socket, uint16 port);

    // 소켓을 수신 대기 상태로 설정
    static bool Listen(SOCKET socket, int backlog = SOMAXCONN);

    // 소켓 옵션 설정
    static bool SetReuseAddr(SOCKET socket);
    static bool SetNoDelay(SOCKET socket);
    static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);

    // Accept된 소켓 옵션 설정
    static bool SetAcceptSockOption(SOCKET acceptedSocket, SOCKET listenSocket);

    // 소캣에서 NetAddress 정보 불러오기
    static bool GetNetAddressBySocket(SOCKET socket, NetAddress& netAddress);

    // 소켓 닫기
    static void CloseSocket(SOCKET socket);

    static LPFN_DISCONNECTEX GetDisconnectEx();
    static LPFN_CONNECTEX GetConnectEx();

private:
    static void LoadDisconnectEx();
    static void LoadConnectEx();

    static LPFN_DISCONNECTEX DisconnectEx;
    static LPFN_CONNECTEX ConnectEx;

};