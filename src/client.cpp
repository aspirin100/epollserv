#include "client.h"

#include <string>
#include <optional>
#include <algorithm>

void ClientInfo::FillMessagesQueue()
{
    while(true)
    {
        size_t pos = buff_.find('\n');
        if(pos == std::string::npos) return;

        msg_queue_.push(buff_.substr(0, pos));
    
        if (!msg.empty() && msg.back() == '\r')
            msg.pop_back();
        
        buff_.erase(0, pos+1);
    }
}

std::optional<std::string> ClientInfo::GetMsgFromQueue()
{
    FillMessagesQueue();

    if(msg_queue_.empty()) return {};

    std::string msg = msg_queue_.front();
    msg_queue_.pop();

    return msg;
}