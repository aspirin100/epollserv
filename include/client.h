#ifndef CLIENT_H
#define CLIENT_H

#include "fd.h"

#include <string>

class ClientInfo
{
private:
    FD fd_;
    std::string to_send_buff_; 
    std::string to_read_buff_;

public:
    explicit ClientInfo(const int fd): fd_{fd} {}
    
    void AppendSendBuff(const std::string& msg) { to_send_buff_.append(msg); }
    void SaveSendBuff(const std::string& msg){ to_send_buff_ = msg; }
    void ClearSendBuff() { to_send_buff_.clear(); }
    bool IsSendBuffEmpty() { return to_send_buff_.empty(); }
    std::string GetSendBuff(){ return to_send_buff_; }

    void AppendReadBuff(const std::string& msg) { to_read_buff_.append(msg); }
    void SaveReadBuff(const std::string& msg){ to_read_buff_ = msg; }
    void ClearReadBuff() { to_read_buff_.clear(); }
    bool IsReadBuffEmpty() { return to_read_buff_.empty(); }
    std::string GetReadBuff(){ return to_read_buff_; }

    int GetFD() { return fd_.GetFD(); }
};

#endif