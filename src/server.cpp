#include "server.h"
#include "fd.h"
#include "event.h"

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
    if(conn_listener_) close(conn_listener_);
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
    event.data.ptr = new ServerEvent(); // TODO: HOW TO CLEAN??
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
    if(event.events & EPOLLERR || event.events & EPOLLHUP)
    {
        std::cout << "epoll error on socket: " << event.data.fd << '\n';
        CloseConnection(static_cast<ClientEvent*>(event.data.ptr)->client);
        return;
    }

    if(event.events & EPOLLIN)
    {
        if(static_cast<BaseEvent*>(event.data.ptr)->type == EventType::SERVER)
        {
            AcceptConnection();
            return;   
        }

        ReadMsg(static_cast<ClientEvent*>(event.data.ptr)->client);
    }
    
    if(event.events & EPOLLOUT)
    {
        auto client = static_cast<ClientEvent*>(event.data.ptr)->client;
        SendMsg(client, client->GetBuff());
    }
}

void Server::CloseConnection(ClientInfo* client)
{
    // probably excess if epoll automatically don't track closed fd
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client->GetFD(), nullptr) < 0)
        perror("failed to remove client from tracked clients");

    delete client;
    --active_clients_count_;
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

    active_clients_[client_fd] = std::make_unique<ClientEvent>(client_fd);

    epoll_event client_event;
    client_event.data.ptr = new ClientEvent(client_fd);
    client_event.events = EPOLLIN;

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &client_event) < 0)
    {
        perror("failed to add client socket into tracking events list");
        return;
    }

    ++active_clients_count_;
    ++total_clients_count_;
}

void Server::ReadMsg(ClientInfo* client)
{
    constexpr int BUFFSIZE = 1 << 14; // 2**14 bytes
    
    while(true)
    {
        char buff[BUFFSIZE];
        int received = recv(client->GetFD(), buff, BUFFSIZE-1, 0);
        
        if(received == 0)
        {
            CloseConnection(client);
            break;
        }

        if(received < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            perror("recv failed");
            CloseConnection(client);
            break;
        }

        buff[received] = '\0';
        ProccessMsg(client, buff);

        // if(buff[received-1] =='\0' || buff[received-1] == '\n')    
        // {
        //     (*client)->AppendReadBuff(buff);
        //     ProccessMsg(client_fd, (*client)->GetReadBuff());           
        //     (*client)->ClearReadBuff();
        // }
        // else
        // {
        //     buff[received] = '\0';
        //     (*client)->AppendReadBuff(buff);
        // }
    } 
}

void Server::ProccessMsg(ClientInfo* client, const std::string& msg)
{
    if(msg == "/stats") 
        SendStats(client);
    else if(msg == "/time")
        SendCurrentTime(client);
    else if(msg == "/shutdown")
        Shutdown();
    else
        SendMsg(client, msg);
}

void Server::SendMsg(ClientInfo* client, const std::string& msg)
{
    client->AppendBuff(msg);
    std::string msg_to_send = client->GetBuff();
    
    int total_sent = 0;
    while(total_sent < msg_to_send.size())
    {
        auto send_pos = msg_to_send.c_str() + total_sent;

        int sent = send(client->GetFD(), send_pos, msg_to_send.size()-total_sent, 0);
        if(sent < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                SaveIntoClientInfoBuff(client, msg_to_send.substr(total_sent));
                break;
            }

            CloseConnection(client);
            perror("message sending error");
            break;
        }

        if(sent == 0)
        {
            CloseConnection(client);
            break;
        }
            
        total_sent += sent;
    }

    if(total_sent == msg_to_send.size())
        ClearClientInfoBuff(client);
}

void Server::SaveIntoClientInfoBuff(ClientInfo* client, const std::string& msg)
{
    client->SaveBuff(msg);

    if(!client->IsBuffEmpty())
    {
        epoll_event client_event;
        client_event.data.ptr = new ClientEvent(client);
        client_event.events = EPOLLIN | EPOLLOUT;

        if(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client->GetFD(), &client_event) < 0)
            perror("failed to add EPOLLOUT into client tracking events");
    }   
}

void Server::ClearClientInfoBuff(ClientInfo* client)
{
    client->ClearBuff();

    epoll_event client_event;
    client_event.data.ptr = new ClientEvent(client);
    client_event.events = EPOLLIN;

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client->GetFD(), &client_event) < 0)
        perror("failed to set EPOLLIN to client tracking events");
}

void Server::SendStats(ClientInfo* client)
{   
    constexpr int BUFFSIZE = 64;
    char buff[BUFFSIZE];

    std::snprintf(buff, BUFFSIZE, "current active clients: %d; total clients: %d", active_clients_.size(), total_clients_count_);

    SendMsg(client, buff);
}

void Server::SendCurrentTime(ClientInfo* client)
{
    const std::string time_format{"2025-11-10 00:00:00"};

    const int BUFFSIZE = time_format.size()+1;
    char buff[BUFFSIZE];

    auto now_timepoint = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now_timepoint);

    std::strftime(buff, BUFFSIZE, "%F %T", std::localtime(&now_time_t));

    SendMsg(client, buff);
}

void Server::Shutdown()
{
    shutdown_requested_ = true;
}