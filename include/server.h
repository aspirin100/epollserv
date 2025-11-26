#ifndef SERVER_H
#define SERVER_H

#include "client.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>

class Server final
{
private:
    std::unordered_map<int, ClientInfo> active_clients_;

    int active_clients_count_ = 0; // TODO: remove
    int total_clients_count_ = 0; // not unique clients total count

    sockaddr_in addr_info_;
    int conn_listener_ = 0;
    int epoll_fd_ = 0;

    bool shutdown_requested_ = false;
public:
    explicit Server(const uint16_t port);

    Server(const Server&) = delete;
    Server(const Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(const Server&&) = delete;
    
    void Start();
    void Shutdown();

    ~Server();
private:
    void AcceptConnection();
    void CloseConnection(const ClientInfo& client);

    void EventLoop();
    void HandleEvent(const epoll_event& event);

    void ReadMsg(ClientInfo& client);
    void SendMsg(ClientInfo& client);
    std::string ProccessMsg(const std::string& msg);

    std::string GetStats();
    std::string GetCurrentTimeStr();

    void SaveIntoClientInfoBuff(ClientInfo& client, const std::string& msg);
    void ClearClientInfoBuff(ClientInfo& client);
};

#endif