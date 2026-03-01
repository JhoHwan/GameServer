#pragma once

class ListenerImpl
{
public:
    static unique_ptr<ListenerImpl> CreateListenerImpl(Listener* owner);

    ListenerImpl(Listener* owner);
    ~ListenerImpl() = default;

    bool StartAccept(const ServerServiceRef & shared);

    void Dispatch(NetEvent * net_event, int32 int32);
};