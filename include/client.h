#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <optional>
#include <queue>
#include <unistd.h>

class ClientInfo
{
private:
    int fd_;
    std::string buff_;
    std::queue<std::string> msg_queue_;

public:
    explicit ClientInfo(const int fd)
        : fd_(fd) {}
    
    void AppendBuff(const std::string& msg) { buff_.append(msg); }
    void SaveBuff(const std::string& msg){ buff_ = msg; }
    void ClearBuff() { buff_.clear(); }
    
    bool IsBuffEmpty() const { return buff_.empty(); }
    std::string GetBuff() const { return buff_; }
    int GetFD() const { return fd_; }

    std::optional<std::string> GetMsgFromQueue();

    ~ClientInfo(){ if(fd_) close(fd_);}
public:
    ClientInfo(const ClientInfo& c) = delete;
    ClientInfo(ClientInfo&& c) = delete;
    ClientInfo& operator=(const ClientInfo& rhs) = delete;
    ClientInfo& operator=(ClientInfo&& rhs) = delete;

private:
    void FillMessagesQueue();
};

#endif