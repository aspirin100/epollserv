#include <iostream>
#include <cstdint>

#include "server.h"

int main()
{
    uint16_t port;
    std::cout << "enter server port: " << std::endl;
    std::cin >> port;

    Server server(port);
    server.Start();

    return 0;
}