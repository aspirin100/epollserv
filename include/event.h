#ifndef EVENT_H
#define EVENT_H

#include "client.h"
#include <memory>

enum class EventType{SERVER, CLIENT};

struct BaseEvent
{
    EventType type;
    int fd;

    BaseEvent(EventType etype, const int fdesc)
        : type(etype), fd(fdesc) {}

    virtual ~BaseEvent() = default;
};

struct ServerEvent : public BaseEvent
{
    ServerEvent(const int fd)
        : BaseEvent(EventType::SERVER, fd){}
};

struct ClientEvent : public BaseEvent
{
    ClientEvent(const int fd)
        : BaseEvent(EventType::CLIENT, fd), client(new ClientInfo(fd)){}
    
    // ClientEvent(ClientInfo* c)
    //     : BaseEvent(EventType::CLIENT), client(c){}
};

#endif