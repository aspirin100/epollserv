#ifndef SERVER_H
#define SERVER_H

#include <cstdint>

class Server
{
private:
    uint16_t port_;

    Server(const uint16_t& port): port_{port}{}
    static Server* server_;

public:
    Server(const Server&) = delete;
    Server(const Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(const Server&&) = delete;
    
    static Server* CreateServer(const uint16_t& port);

    ~Server();
};

#endif