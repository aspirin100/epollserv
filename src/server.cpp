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
      perror("failed to open socket: ");
      return std::nullopt;
    } 

    if(server_->epoll_fd_.fd = epoll_create1(0); server_->epoll_fd_.fd < 0)
    {
      perror("failed to open epoll sock: ");
      return std::nullopt;
    }  

    return server_;
}

Server::~Server()
{
  if(server_) delete server_;
}

void Server::Start()
{
    if(bind(conn_listener_.fd, reinterpret_cast<sockaddr*>(addr_info_.get()), sizeof(*addr_info_)) < 0)
    {
        perror("failed to bind the socket:");
        return;
    } 

    if(listen(conn_listener_.fd, SOMAXCONN) < 0)
    {
        perror("error on listen():");
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
        perror("failed to add listener socket into tracking events list:");
        return;
    }

    while(true)
    {
        int n = epoll_wait(epoll_fd_.fd, catched_events, MAX_EVENTS, -1);

        if(n < 0)
        {
            perror("epoll_wait fail:");
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
        std::cerr << "epoll error on socket: " << event.data.fd << '\n';
        CloseConnection(event.data.fd);
        return;
    }

    if(event.data.fd == conn_listener_.fd)
    {
        AcceptConnection();
        return;   
    }

    std::string msg = ReadMsg(event.data.fd);
    ProccessMsg(event.data.fd, msg);
}

void Server::CloseConnection(const int& fd)
{
    active_clients_.erase(fd);
}

void Server::AcceptConnection()
{
    int client_fd = accept(conn_listener_.fd, nullptr, nullptr);
    if(client_fd < 0)
    {
        perror("accept fail:");
        return;
    }

    active_clients_.emplace(client_fd);
    ++total_clients_;
}

std::string Server::ReadMsg(const int& client_fd)
{
    auto client = active_clients_.find(client_fd);
    if(client == active_clients_.end())
        return {};
    
    constexpr int BUFFSIZE = 128;
    char buff[BUFFSIZE];
    std::string msg;

    int received = recv(client_fd, buff, BUFFSIZE, 0);
    while(received > 0)
        msg.append(buff);

    return msg;
}

void Server::ProccessMsg(const int& client_fd, const std::string& msg)
{
    std::cout << "message is: " << msg << '\n';
}