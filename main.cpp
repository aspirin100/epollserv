#include <iostream>
#include <memory>
#include <errno.h>
#include <cstdint>
#include <sys/epoll.h>
#include <unistd.h>

#include "server.h"

int main()
{
    uint16_t port = 8888;
    Server* serv = Server::CreateServer(port);

    // auto fdcloser = [](const int* fd){
    //     if(fd)
    //         close(*fd);
    //     delete fd;
    // };
    // std::unique_ptr<int, decltype(fdcloser)> epoll_fd(new int, fdcloser);

    // if(*epoll_fd = epoll_create(18); *epoll_fd < 0)
    // {
    //     perror("failed to create epoll instance:");
    //     return -1;
    // }

    // std::cout << "epoll fd: " << *epoll_fd << '\n';



    return 0;
}