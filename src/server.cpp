#include "server.h"

#include <cstdint>
#include <optional>
#include <memory>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
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
    if(!server_)
        server_ = new Server(port);

    if(server_->listener_fd_ = socket(AF_INET, SOCK_STREAM, 0); server_->listener_fd_ < 0)
    {
      perror("failed to open socket: ");
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
    if(bind(listener_fd_, reinterpret_cast<sockaddr*>(addr_info_.get()), sizeof(*addr_info_)) < 0)
    {
        perror("failed to bind the socket:");
        return;
    } 

    if(listen(listener_fd_, SOMAXCONN) < 0)
    {
        perror("error on listen():");
        return;
    }

    auto socket_closer = [](int* fd)
    {
        if(fd)
            close(*fd);
        delete(fd);
    };

    while(true)
    {
        auto client_sfd = std::unique_ptr<int, decltype(socket_closer)>(new int, socket_closer);

        if(*client_sfd = accept(listener_fd_, nullptr, nullptr); *client_sfd < 0)
        {
            perror("accept() failed:");
            continue;
        }
        
        char buff[1024];
        while(true)
        {
            int readed = recv(*client_sfd, buff, 1024, 0);

            if(readed <= 0) break;
            printf("readed from client: %s\n", buff);
            send(*client_sfd, buff, readed, 0);
        }
    }
}
