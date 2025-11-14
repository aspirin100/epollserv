#include "server.h"
#include <cstdint>

Server* Server::server_ = nullptr;
Server* Server::CreateServer(const uint16_t& port){
    if(!server_)
        server_ = new Server(port);

    return server_;
}

Server::~Server(){if(server_) delete server_;}
