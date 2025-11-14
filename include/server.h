#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <memory>
#include <sys/types.h>
#include <netinet/in.h>

class Server
{
private:
    std::unique_ptr<sockaddr_in> addr_info_; 

    Server(const uint16_t& port);
    static Server* server_;

public:
    Server(const Server&) = delete;
    Server(const Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(const Server&&) = delete;
    
    static Server* CreateServer(const uint16_t& port);

    ~Server();

    void Start();
};

#endif