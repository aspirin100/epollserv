#include "server.h"

#include <iostream>
#include <chrono>

#include <optional>
#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <ctime>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
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

    if(tcp_listener_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); tcp_listener_fd_ < 0)
    {
      perror("failed to open tcp socket");
      return;
    } 

    if(udp_listener_fd_ = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0); udp_listener_fd_ < 0)
    {
      perror("failed to open udp socket");
      return;
    } 

    if(timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK); timer_fd_ < 0)
    {
        perror("failed to create timerfd instance");
        return;
    }

    int opt = 1;

    setsockopt(tcp_listener_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(udp_listener_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(epoll_fd_ = epoll_create1(0); epoll_fd_ < 0)
      perror("failed to open epoll sock");
}

Server::~Server()
{
    if(tcp_listener_fd_) close(tcp_listener_fd_);
    if(udp_listener_fd_) close(udp_listener_fd_);
    if(timer_fd_) close(timer_fd_);
    if(epoll_fd_) close(epoll_fd_);
}

void Server::Start()
{
    // tcp socket 
    if(bind(tcp_listener_fd_, reinterpret_cast<sockaddr*>(&addr_info_), sizeof(addr_info_)) < 0)
    {
        perror("failed to bind tcp socket");
        return;
    } 

    if(listen(tcp_listener_fd_, SOMAXCONN) < 0)
    {
        perror("error on listen()");
        return;
    }

    // udp socket
    if(bind(udp_listener_fd_, reinterpret_cast<sockaddr*>(&addr_info_), sizeof(addr_info_)) < 0)
    {
        perror("failed to bind udp socket");
        return;
    } 

    // timer setup
    if(!SetupUdpClientsClearTimer())
        return;

    // adding tracking events on sockets
    if(!AddTrackingEvents(tcp_listener_fd_, EPOLLIN))
        return;
        
    if(!AddTrackingEvents(udp_listener_fd_, EPOLLIN | EPOLLERR))
        return;
   
    if(!AddTrackingEvents(timer_fd_, EPOLLIN))
        return;


    EventLoop();
}

bool Server::AddTrackingEvents(const int fd, const int events)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = events;

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("EPOLL_CTL_ADD error");
        return false;
    }

    return true;
}

void Server::EventLoop()
{
    constexpr int MAX_EVENTS = 64;
    epoll_event catched_events[MAX_EVENTS];

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

    if(event.data.fd == timer_fd_)
    {
        HandleTimerEvent(event);
        return;
    }

    if(event.data.fd == udp_listener_fd_)
    {
        HandleUdpEvent(event);
        return;
    }

    if(event.data.fd == tcp_listener_fd_ && event.events & EPOLLIN)
    {
        AcceptConnection();
        return;   
    }

    auto client = active_tcp_clients_.find(event.data.fd);
    if(client == active_tcp_clients_.end()) return;

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

    active_tcp_clients_.erase(client_fd);
}

void Server::AcceptConnection()
{
    int client_fd = accept(tcp_listener_fd_, nullptr, nullptr);
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
    
    active_tcp_clients_.emplace(client_fd, client_fd);
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
        client.to_read_buff.append(std::string(buff, received));

        while(true)
        {
            auto msg = client.ExtractMsg();
            if(!msg) break;
        
            auto proccessed_msg = ProccessMsg(msg.value());
            if(!proccessed_msg) continue;

            if (!SendMsg(client, proccessed_msg.value())) return;
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
    {   
        if(!cmd.empty() && cmd.front() == '/')
            return cmd + " is not supported command";

        return cmd;
    }
        
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
            return false;
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

void Server::ModifyClientEvent(const int client_fd, const int events)
{
    epoll_event client_event;
    client_event.data.fd = client_fd;
    client_event.events = events;

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &client_event) < 0)
        perror("failed to modify client event");
}

std::string Server::GetStats()
{   
    constexpr int BUFFSIZE = 64;
    char buff[BUFFSIZE];

    std::snprintf(buff, BUFFSIZE, "current active clients: %d; total clients: %d", active_tcp_clients_.size() + active_udp_clients_.size(), total_clients_count_);

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

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, tcp_listener_fd_, nullptr) < 0)
            perror("failed to remove tcp listener from tracked event list");

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, udp_listener_fd_, nullptr) < 0)
        perror("failed to remove udp listener from tracked event list");

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, timer_fd_, nullptr) < 0)
        perror("failed to remove timer from tracked event list");

    if(close(tcp_listener_fd_) < 0) perror("tcp listener socket close fail");
    tcp_listener_fd_ = -1;

    if(close(udp_listener_fd_) < 0) perror("udp listener socket close fail");
    udp_listener_fd_ = -1;

    if(close(timer_fd_) < 0) perror("timer close fail");
    timer_fd_ = -1;

    for(const auto &[fd, client] : active_tcp_clients_)
        if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0)
            perror("failed to remove client from tracked clients");
    
    active_tcp_clients_.clear();
    
    if(close(epoll_fd_) < 0) perror("epoll fd close fail");
    epoll_fd_ = -1;
}