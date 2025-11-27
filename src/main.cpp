#include <iostream>
#include <cstdint>
#include <cstdlib>

#include "server.h"

int main(int argc, char **argv)
{
    uint16_t port = 0;

    if(argc > 1)
        port = std::atoi(argv[1]);
    
    if(!port)
        port = 8888;
    
    Server server(port);
    server.Start();

    return 0;
}