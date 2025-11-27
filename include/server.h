#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <optional>

#include <cstdint>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>

class Server final
{
private:
    class ClientInfo
    {
    public:
        int fd;

        std::string to_read_buff;
        std::string to_write_buff;

        std::queue<std::string> msg_queue;

        explicit ClientInfo(const int fdesc)
            : fd(fdesc) {}

        std::optional<std::string> GetMsgFromQueue();

        ClientInfo(const ClientInfo& c) = delete;
        ClientInfo(ClientInfo&& c) = delete;
        ClientInfo& operator=(const ClientInfo& rhs) = delete;
        ClientInfo& operator=(ClientInfo&& rhs) = delete;
        
        ~ClientInfo();
    private:
        void FillMessagesQueue();
    };

    std::unordered_map<int, ClientInfo> active_tcp_clients_;
    std::unordered_set<std::string> active_udp_clients_; // clears every 5 minute

    int total_clients_count_ = 0; // not unique clients total count

    sockaddr_in addr_info_;

    int tcp_listener_fd_ = -1;
    int udp_listener_fd_ = -1;
    int timer_fd_ = -1;
    int epoll_fd_ = -1;

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
    void CloseConnection(const int client_fd);

    void EventLoop();
    void HandleEvent(const epoll_event& event);
    void HandleUdpEvent(const epoll_event& event);
    void HandleTimerEvent();

    void ReadMsg(ClientInfo& client);
    bool SendMsg(ClientInfo& client, const std::string& msg);
    void UdpReadWrite();

    std::optional<std::string> ProccessMsg(const std::string& msg);
    std::optional<std::string> ProccessUdpMsg(const std::string& msg);

    std::string GetStats();
    std::string GetCurrentTimeStr();

    void SaveIntoClientWriteBuff(ClientInfo& client, const std::string& msg);
    void ClearClientWriteBuff(ClientInfo& client);

    void ModifyClientEvent(const int fd, const int events);
    bool AddTrackingEvents(const int fd, const int events);
    bool SetupUdpClientsClearTimer();
};

#endif