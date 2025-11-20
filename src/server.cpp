#include "server.h"
#include "fd.h"

#include <iostream>
#include <cstdint>
#include <optional>
#include <memory>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>

Server::Server(const uint16_t& port): addr_info_{new sockaddr_in}
{
    addr_info_->sin_port = htons(port);
    addr_info_->sin_family = AF_UNSPEC;
    addr_info_->sin_addr.s_addr = INADDR_ANY;   
}

Server* Server::server_ = nullptr;
std::optional<Server*> Server::CreateServer(const uint16_t& port){
    if(server_)
        return server_;
       
    server_ = new Server(port);

    if(server_->conn_listener_.fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); server_->conn_listener_.fd < 0)
    {
      perror("failed to open socket");
      return std::nullopt;
    } 

    if(server_->epoll_fd_.fd = epoll_create1(0); server_->epoll_fd_.fd < 0)
    {
      perror("failed to open epoll sock");
      return std::nullopt;
    }  

    return server_;
}

Server::~Server()
{   
    if(server_)
        delete server_;
}

void Server::Start()
{
    if(bind(conn_listener_.fd, reinterpret_cast<sockaddr*>(addr_info_.get()), sizeof(*addr_info_)) < 0)
    {
        perror("failed to bind the socket");
        return;
    } 

    if(listen(conn_listener_.fd, SOMAXCONN) < 0)
    {
        perror("error on listen()");
        return;
    }

    EventLoop();
}

void Server::EventLoop()
{
    epoll_event event;
    event.data.fd = conn_listener_.fd;
    event.events = EPOLLIN;

    constexpr int MAX_EVENTS = 64;
    epoll_event catched_events[MAX_EVENTS];

    if(epoll_ctl(epoll_fd_.fd, EPOLL_CTL_ADD, conn_listener_.fd, &event) < 0)
    {
        perror("failed to add listener socket into tracking events list");
        return;
    }

    while(true)
    {
        int n = epoll_wait(epoll_fd_.fd, catched_events, MAX_EVENTS, -1);

        if(n < 0)
        {
            perror("epoll_wait fail");
            continue;
        }

        for(int i = 0; i < n; ++i)
        {
            HandleEvent(catched_events[i]); // handler
        }
    }
}

void Server::HandleEvent(const epoll_event& event)
{
    if(event.events & EPOLLERR ||
        event.events & EPOLLHUP ||
        !(event.events & EPOLLIN))
    {
        std::cout << "epoll error on socket: " << event.data.fd << '\n';
        CloseConnection(event.data.fd);
        return;
    }

    if(event.data.fd == conn_listener_.fd)
    {
        AcceptConnection();
        return;   
    }

    ReadMsg(event.data.fd);
}

void Server::CloseConnection(const int& fd)
{
    if(epoll_ctl(epoll_fd_.fd, EPOLL_CTL_DEL, fd, nullptr) < 0)
        perror("failed to remove client from tracked clients");

    active_clients_.erase(fd);
}

void Server::AcceptConnection()
{
    int client_fd = accept(conn_listener_.fd, nullptr, nullptr);
    if(client_fd < 0)
    {
        perror("accept fail");
        return;
    }

    epoll_event client_event;
    client_event.data.fd = client_fd;
    client_event.events = EPOLLIN;

    if(epoll_ctl(epoll_fd_.fd, EPOLL_CTL_ADD, client_fd, &client_event) < 0)
    {
        perror("failed to add client socket into tracking events list");
        return;
    }

    active_clients_.emplace(client_fd, client_fd);
    ++total_clients_;
}

void Server::ReadMsg(const int& client_fd)
{
    constexpr int BUFFSIZE = 128;
    while(true)
    {
        char buff[BUFFSIZE];
        int received = recv(client_fd, buff, BUFFSIZE-1, 0);
        
        if(received == 0)
        {
            CloseConnection(client_fd);
            break;
        }

        if(received < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            perror("recv failed");
            CloseConnection(client_fd);
            break;
        }

        buff[received] = '\0';
        ProccessMsg(client_fd, buff);   
    }
}

void Server::ProccessMsg(const int& client_fd, const std::string& msg)
{
    if(msg == "/stats") 
        ShowStats(client_fd);
    else if(msg == "/time")
        GetTime(client_fd);
    else if(msg == "/shutdown")
        Shutdown();
    else
        SendMsg(client_fd, msg);
}

void Server::ShowStats(const int& client_fd){}
void Server::GetTime(const int& client_fd){}
void Server::Shutdown(){}


void Server::SendMsg(const int&  client_fd, const std::string& msg)
{
    auto client = GetClient(client_fd);
    if(!client)
        return;

    std::string msg_to_send = (*client)->GetSendBuff() + msg;

    if(!(*client)->IsBuffEmpty())
        ClearClientBuff(*(client.value()));

    int total_sent = 0;

    while(total_sent < msg_to_send.size())
    {
        auto send_pos = msg_to_send.c_str() + (sizeof(char)*total_sent);

        int sent = send(client_fd, send_pos, 5, 0);
        if(sent < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                SaveIntoClientBuff(*(client.value()), msg);
                break;
            }

            CloseConnection(client_fd);
            perror("message sending error");
            break;
        }

        if(sent == 0)
            break;

        total_sent += sent;
    }
}

std::optional<Client*> Server::GetClient(const int& fd)
{
    auto it = active_clients_.find(fd);
    if(it == active_clients_.end())
        return std::nullopt;

    return &(it->second);
}

void Server::SaveIntoClientBuff(Client& client, const std::string& msg)
{
    epoll_event client_event;
    client_event.data.fd = client.GetFD();
    client_event.events = EPOLLIN | EPOLLOUT;

    if(epoll_ctl(epoll_fd_.fd, EPOLL_CTL_MOD, client.GetFD(), &client_event) < 0)
    {
        perror("failed to add EPOLLOUT into client tracking events");
        return;
    }

    client.SaveBuff(msg);
}

void Server::ClearClientBuff(Client& client)
{
    epoll_event client_event;
    client_event.data.fd = client.GetFD();
    client_event.events = EPOLLIN;

    if(epoll_ctl(epoll_fd_.fd, EPOLL_CTL_MOD, client.GetFD(), &client_event) < 0)
    {
        perror("failed to set EPOLLIN to client tracking events");
        return;
    }

    client.ClearBuff();
}