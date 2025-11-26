#include "server.h"

#include <iostream>
// for uint16_t-like types
#include <cstdint>

// for time outputting
#include <chrono>
#include <ctime>

#include <optional>
#include <memory>

#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>


Server::Server(const uint16_t port): addr_info_{sockaddr_in{}}
{
    addr_info_.sin_port = htons(port);
    addr_info_.sin_family = AF_INET;
    addr_info_.sin_addr.s_addr = INADDR_ANY;   

    if(conn_listener_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); conn_listener_ < 0)
    {
      perror("failed to open socket");
      return;
    } 

    int opt = 1;
    setsockopt(conn_listener_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(epoll_fd_ = epoll_create1(0); epoll_fd_ < 0)
      perror("failed to open epoll sock");
}

Server::~Server()
{
    if(conn_listener_) close(conn_listener_);
    if(epoll_fd_) close(epoll_fd_);
}

void Server::Start()
{
    if(bind(conn_listener_, reinterpret_cast<sockaddr*>(&addr_info_), sizeof(addr_info_)) < 0)
    {
        perror("failed to bind the socket");
        return;
    } 

    if(listen(conn_listener_, SOMAXCONN) < 0)
    {
        perror("error on listen()");
        return;
    }

    EventLoop();
}

void Server::EventLoop()
{
    epoll_event event;
    event.data.fd = conn_listener_;
    event.events = EPOLLIN;

    constexpr int MAX_EVENTS = 64;
    epoll_event catched_events[MAX_EVENTS];

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, conn_listener_, &event) < 0)
    {
        perror("failed to add listener socket into tracking events list");
        return;
    }

    while(!shutdown_requested_)
    {
        int n = epoll_wait(epoll_fd_, catched_events, MAX_EVENTS, -1);

        if(n < 0)
        {
            perror("epoll_wait fail");
            continue;
        }

        for(int i = 0; i < n; ++i)
        {
            HandleEvent(catched_events[i]);
        }
    }
}

void Server::HandleEvent(const epoll_event& event)
{   
    if(shutdown_requested_) return;

    if(event.data.fd == conn_listener_ && event.events & EPOLLIN)
    {
        AcceptConnection();
        return;   
    }

    auto client = active_clients_.find(event.data.fd);
    if(client == active_clients_.end()) return;

    if(event.events & EPOLLERR || event.events & EPOLLHUP)
    {
        std::cout << "epoll error on socket: " << event.data.fd << '\n';
        CloseConnection(client->second.fd);
        return;
    }

    if(event.events & EPOLLIN)
        ReadMsg(client->second);
    
    if(event.events & EPOLLOUT)
        SendMsg(client->second, client->second.to_write_buff);
}

void Server::CloseConnection(const int client_fd)
{
    // probably excess if epoll automatically don't track closed fd
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr) < 0)
        perror("failed to remove client from tracked clients");

    active_clients_.erase(client_fd);
}

void Server::AcceptConnection()
{
    int client_fd = accept(conn_listener_, nullptr, nullptr);
    if(client_fd < 0)
    {
        perror("accept fail");
        return;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    epoll_event client_event;
    client_event.data.fd = client_fd;
    client_event.events = EPOLLIN;
    
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &client_event) < 0)
    {
        perror("failed to add client socket into tracking events list");
        return;
    }
    
    active_clients_.emplace(client_fd, client_fd);
    ++total_clients_count_;
}

void Server::ReadMsg(ClientInfo& client)
{
    constexpr int BUFFSIZE = 1 << 14; // 2**14 bytes
    char buff[BUFFSIZE];
    
    while(true)
    {
        int received = recv(client.fd, buff, BUFFSIZE-1, 0);
        
        if(received == 0)
        {
            CloseConnection(client.fd);
            break;
        }

        if(received < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            perror("recv failed");
            CloseConnection(client.fd);
            break;
        }

        buff[received] = '\0';
        client.to_read_buff = std::string(buff, received);

        while(true)
        {
            auto msg = client.GetMsgFromQueue();
            if(!msg) break;
        
            auto proccessed_msg = ProccessMsg(msg.value());
            if(!proccessed_msg) continue;

            if (!SendMsg(client, proccessed_msg.value())) break;
        }
    } 
}

std::optional<std::string> Server::ProccessMsg(const std::string& cmd)
{
    if(cmd == "/shutdown")
    {
        Shutdown();
        return std::nullopt;
    }

    if(cmd == "/stats") 
        return GetStats();
    if(cmd == "/time")
        return GetCurrentTimeStr();
    else
        return cmd;
}

bool Server::SendMsg(ClientInfo& client, const std::string& msg)
{
    int total_sent = 0;
    while(total_sent < msg.size())
    {
        auto send_pos = msg.c_str() + total_sent;

        int sent = send(client.fd, send_pos, msg.size()-total_sent, 0);
        if(sent < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                client.to_write_buff = msg.substr(total_sent);
                ModifyClientEvent(client.fd, EPOLLIN | EPOLLOUT);

                return false;
            }

            CloseConnection(client.fd);
            perror("message sending error");
            break;
        }

        if(sent == 0)
        {
            CloseConnection(client.fd);
            return false;
        }
            
        total_sent += sent;
    }

    if(total_sent == msg.size())
    {
        client.to_write_buff.clear();
        ModifyClientEvent(client.fd, EPOLLIN);
    }

    return true;
}

void Server::ModifyClientEvent(const int fd, int flag)
{
    epoll_event client_event;
    client_event.data.fd = fd;
    client_event.events = flag;

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &client_event) < 0)
        perror("failed to modify client event");
}

std::string Server::GetStats()
{   
    constexpr int BUFFSIZE = 64;
    char buff[BUFFSIZE];

    std::snprintf(buff, BUFFSIZE, "current active clients: %d; total clients: %d", active_clients_.size(), total_clients_count_);

    return buff;
}

std::string Server::GetCurrentTimeStr()
{
    const std::string time_format{"2025-11-10 00:00:00"};

    const int BUFFSIZE = time_format.size()+1;
    char buff[BUFFSIZE];

    auto now_timepoint = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now_timepoint);

    std::strftime(buff, BUFFSIZE, "%F %T", std::localtime(&now_time_t));

    return buff;
}

void Server::Shutdown()
{
    shutdown_requested_ = true;

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn_listener_, nullptr) < 0)
            perror("failed to remove server from tracked event list");

    if(close(conn_listener_) < 0) perror("listener socket close fail");
    conn_listener_ = -1;

    for(const auto &[fd, client] : active_clients_)
        if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0)
            perror("failed to remove client from tracked clients");
    
    active_clients_.clear();
    
    if(close(epoll_fd_) < 0) perror("epoll fd close fail");
    epoll_fd_ = -1;
}