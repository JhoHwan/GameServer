#include "Listener_Linux.h"
#include "Listener.h"

ListenerImpl_Linux::ListenerImpl_Linux(Listener *owner) :_owner(owner)
{
}

bool ListenerImpl_Linux::StartAccept(const ServerServiceRef& service)
{
}

void ListenerImpl_Linux::Dispatch(NetEvent *net_event, int32 int32)
{
}
