#ifndef SERVER_H
#define SERVER_H

#include "fd.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>

class Server final
{
private:
    std::unordered_set<int> active_clients_;
    int total_clients_ = 0; // not unique total clients count

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
    void AcceptConnection();
    void CloseConnection(const int& fd);

    void EventLoop();
    void HandleEvent(const epoll_event& event);
    std::string ReadMsg(const int& client_fd);
    void ProccessMsg(const int& client_fd, const std::string& msg);
};

#endif