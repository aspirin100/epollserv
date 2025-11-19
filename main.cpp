#include <iostream>
#include <memory>
#include <errno.h>
#include <cstdint>
#include <sys/epoll.h>
#include <unistd.h>

#include "server.h"

int main()
{
    constexpr uint16_t port = 8888;
    
    std::optional<Server*> serv = Server::CreateServer(port);
    if(!serv)
    {
        std::cout << "failed to create server\n";
        return -1;
    }

    (*serv)->Start();

    return 0;
}