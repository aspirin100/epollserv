#include "server.h"

#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <arpa/inet.h>

void Server::HandleUdpEvent(const epoll_event& event)
{
    if(event.events & EPOLLERR)
    {
        int err = 0;
        socklen_t len = sizeof(err);

        if(getsockopt(udp_listener_fd_, SOL_SOCKET, SO_ERROR, &err, &len) < 0) return;
        
        if(err > 0) std::cout << "udp event error: " << strerror(err) << std::endl;
        else std::cout << "udp event error\n";

        return;
    }
    
    if(event.events & EPOLLIN)
        UdpReadWrite();
}

void Server::UdpReadWrite()
{
    constexpr int BUFFSIZE = 1<<16; // 2**16 bytes
    char buff[BUFFSIZE];

    sockaddr_in client{};
    socklen_t socklen = sizeof(client);

    while(true)
    {
        int received = recvfrom(udp_listener_fd_, buff, BUFFSIZE-1, 0, reinterpret_cast<sockaddr*>(&client), &socklen);

        if(received < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            perror("recvfrom error");
            break;
        }

        buff[received] = '\0';
        auto proccessed_msg = ProccessUdpMsg(std::string(buff, received));

        if(!proccessed_msg) continue;

        int sent = sendto(udp_listener_fd_, proccessed_msg.value().c_str(), proccessed_msg.value().size(), 0, reinterpret_cast<sockaddr*>(&client), socklen);
        if(sent < 0)
            perror("sendto err");
    }

    std::string client_ip = inet_ntoa(client.sin_addr);
    uint16_t client_port = htons(client.sin_port);

    int previous_clients_count = active_udp_clients_.size();

    active_udp_clients_.emplace(client_ip+':'+std::to_string(client_port));

    if(active_udp_clients_.size() > previous_clients_count) ++total_clients_count_;
}

std::optional<std::string> Server::ProccessUdpMsg(const std::string& msg)
{
    std::string formatted = msg;

    while(!formatted.empty() && formatted.back() == '\n' || formatted.back() == '\r')
        formatted.pop_back();

    return ProccessMsg(formatted);
}

bool Server::SetupUdpClientsClearTimer()
{   
    itimerspec timer;

    timer.it_value.tv_sec = 60*5;
    timer.it_value.tv_nsec = 0;
    timer.it_interval.tv_sec = 60*5;
    timer.it_interval.tv_nsec = 0;
    
    if(timerfd_settime(timer_fd_, 0, &timer, nullptr) < 0)
    {
        perror("timer setup fail");
        return false;
    }

    return true;
}

void Server::HandleTimerEvent()
{
    active_udp_clients_.clear();
}