#pragma once

class ListenerImpl
{
public:
    static unique_ptr<ListenerImpl> CreateListenerImpl(Listener* owner);

    ListenerImpl(Listener* owner);
    ~ListenerImpl() = default;

public:
    bool StartAccept(const ServerServiceRef& service);
    void Dispatch(NetEvent* netEvent, int32 numOfBytes = 0);

private:
    void RegisterAccept(AcceptEvent* acceptEvent);
    void ProcessAccept(AcceptEvent* acceptEvent);

private:
    Listener* _owner;

};