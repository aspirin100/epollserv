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

    serv->Start();

    return 0;
}