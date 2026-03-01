#pragma once

class ListenerImpl_Win
{
public:
    static unique_ptr<ListenerImpl_Win> CreateListenerImpl(Listener* owner)
    {
        return make_unique<ListenerImpl_Win>(owner);
    }

    ListenerImpl_Win(Listener* owner);
    ~ListenerImpl_Win() = default;

public:
    bool StartAccept(const ServerServiceRef& service);
    void Dispatch(NetEvent* netEvent, int32 numOfBytes = 0);
    void Close();

private:
    void RegisterAccept(AcceptEvent* acceptEvent);
    void ProcessAccept(AcceptEvent* acceptEvent);

private:
    Listener* _owner;

};