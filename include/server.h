#ifndef SERVER_H
#define SERVER_H

#include "FD.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>

class Server final
{
private:
    std::unique_ptr<sockaddr_in> addr_info_ = nullptr;
    FD conn_listener_{-1};
    FD epoll_fd_{-1};

    Server(const uint16_t& port);
    static Server* server_;

public:
    Server(const Server&) = delete;
    Server(const Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(const Server&&) = delete;
    
    static std::optional<Server*> CreateServer(const uint16_t& port);

    ~Server();

    void Start();

private:
    void EventLoop();
    void HadleEvent(const epoll_event& event);
    void ProccessMsg(); // TODO: define appropriate type for messages
    // TODO: connection closing
    // TODO: probably handler interface for message handling
};

#endif