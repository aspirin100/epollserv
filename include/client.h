#ifndef CLIENT_H
#define CLIENT_H

#include "fd.h"

#include <string>

class Client
{
private:
    FD fd_;
    std::string send_buff_; 

public:
    explicit Client(const int& fd): fd_{fd} {}
    void SaveBuff(const std::string& msg){ send_buff_ = msg; }
    void ClearBuff() { send_buff_.clear(); }

    bool IsBuffEmpty() { return send_buff_.empty(); }
    std::string GetSendBuff(){ return send_buff_; }

    int GetFD() { return fd_.fd; }
};



#endif