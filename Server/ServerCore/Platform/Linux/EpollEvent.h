#pragma once

enum class EventType : uint8;

class EpollEvent
{
public:
    EpollEvent(EventType type) : eventType(type) { Init(); }
    void Init() { /* Epoll은 특별히 초기화할 멤버가 없으면 비워둠 */ }

public:
    EventType eventType;
    NetObjectRef owner;
};