#ifndef SERVER_H
#define SERVER_H

#include "fd.h"
#include "client.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>

class Server final
{
private:
    int active_clients_count_ = 0;
    int total_clients_count_ = 0; // not unique clients total  count

    sockaddr_in addr_info_;
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
    void CloseConnection(ClientInfo* client);

    void EventLoop();
    void HandleEvent(const epoll_event& event);
    void ReadMsg(ClientInfo* client);
    void ProccessMsg(ClientInfo* client, const std::string& msg);

    void SendStats(ClientInfo* client);
    void SendCurrentTime(ClientInfo* client);
    void Shutdown();
    void SendMsg(ClientInfo* client, const std::string& msg);

    void SaveIntoClientInfoBuff(ClientInfo* client, const std::string& msg);
    void ClearClientInfoBuff(ClientInfo* client);
};

#endif