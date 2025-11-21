#include <iostream>
#include <memory>

#include "server.h"

int main()
{
    std::unique_ptr<Server> server(new Server(8888));
    server->Start();

    std::cout << "correctly shutdowned\n";
    return 0;
}