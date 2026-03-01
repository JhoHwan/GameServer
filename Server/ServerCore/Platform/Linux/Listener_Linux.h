#pragma once

class ListenerImpl_Linux
{
public:
    static unique_ptr<ListenerImpl_Linux> CreateListenerImpl(Listener* owner)
    {
        return make_unique<ListenerImpl_Linux>(owner);
    }


    ListenerImpl_Linux(Listener* owner);
    ~ListenerImpl_Linux() = default;

    bool StartAccept(const ServerServiceRef& service);

    void Dispatch(NetEvent * net_event, int32 int32);

private:
    Listener* _owner;
};