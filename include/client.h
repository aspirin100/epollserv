#ifndef CLIENT_H
#define CLIENT_H

#include "fd.h"

#include <string>
#include <unistd.h>

class ClientInfo
{
private:
    int fd_;
    std::string buff_;

public:
    explicit ClientInfo(const int fd)
        : fd_(fd) {}
    
    void AppendBuff(const std::string& msg) { buff_.append(msg); }
    void SaveBuff(const std::string& msg){ buff_ = msg; }
    void ClearBuff() { buff_.clear(); }
    bool IsBuffEmpty() { return buff_.empty(); }
    std::string GetBuff(){ return buff_; }

    int GetFD() { return fd_; }

    ~ClientInfo(){ if(fd_) close(fd_);}
};

#endif