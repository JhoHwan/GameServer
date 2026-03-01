#include "Listener_Linux.h"
#include "Listener.h"
#include "SocketUtils.h"

ListenerImpl_Linux::ListenerImpl_Linux(Listener *owner) :_owner(owner)
{
}

bool ListenerImpl_Linux::StartAccept(const ServerServiceRef& service)
{
    return true;
}

void ListenerImpl_Linux::ProcessAccept()
{
    while(true)
    {
        SOCKADDR_IN clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept4(_owner->GetHandle(), reinterpret_cast<SOCKADDR*>(&clientAddr), &clientAddrLen, SOCK_NONBLOCK);

        if (clientSocket == INVALID_SOCKET)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;

            perror("ListenerImpl_Linux::ProcessAccept");
            return;
        }

        // TODO : Session 변경 - Session 생성자에 SOCKET 입력, 세션 생성
        SessionRef session;
        cout << "Connected Client FD: " << clientSocket << endl;

        session->SetNetAddress(NetAddress(clientAddr));
        session->ProcessConnect();
    }
}

void ListenerImpl_Linux::Dispatch(NetEvent* netEvent, int32 int32)
{
    const uint32 eventFlags = netEvent->eventFlags;

    if (eventFlags & EPOLLIN)
    {
        ProcessAccept();
    }

    if (eventFlags & (EPOLLERR | EPOLLHUP))
    {
        perror("ListenerImpl_Linux::Dispatch");
    }
}

void ListenerImpl_Linux::Close()
{
    SocketUtils::Close(_owner->_socket);
    _owner->_epollRef.reset();
}
