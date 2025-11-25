#ifndef SERVER_H
#define SERVER_H

#include "fd.h"
#include "client.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>

class Server final
{
private:
    std::unordered_map<int, ClientInfo> active_clients_;
    int total_clients_ = 0; // not unique total clients count

    std::unique_ptr<sockaddr_in> addr_info_ = nullptr;
    FD conn_listener_{-1};
    FD epoll_fd_{-1};

    bool shutdown_requested_ = false;
public:
    explicit Server(const uint16_t port);

    Server(const Server&) = delete;
    Server(const Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(const Server&&) = delete;
    
    void Start();

private:
    void AcceptConnection();
    void CloseConnection(const int fd);

    void EventLoop();
    void HandleEvent(const epoll_event& event);
    void ReadMsg(const int client_fd);
    void ProccessMsg(const int client_fd, const std::string& msg);

    void SendStats(const int client_fd);
    void SendCurrentTime(const int client_fd);
    void Shutdown();
    void SendMsg(const int client_fd, const std::string& str);

    std::optional<ClientInfo*> GetClientInfo(const int client_fd);
    void SaveIntoClientInfoBuff(ClientInfo& client, const std::string& msg);
    void ClearClientInfoBuff(ClientInfo& client);
};

#endif