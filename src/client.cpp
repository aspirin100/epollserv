#include "server.h"

#include <string>
#include <optional>
#include <algorithm>
#include <unistd.h>

#include <iostream>

void Server::ClientInfo::FillMessagesQueue()
{
    while(true)
    {
        size_t pos = to_read_buff.find('\n');
        if(pos == std::string::npos)
            return;
        

        std::string msg = to_read_buff.substr(0, pos);
        
        if (!msg.empty() && msg.back() == '\r')
            msg.pop_back();

        to_read_buff.erase(0, pos+1);
    
        msg_queue.push(msg);
    }
}

std::optional<std::string> Server::ClientInfo::GetMsgFromQueue()
{
    FillMessagesQueue();

    if(msg_queue.empty())
        return std::nullopt;  
    
    std::string msg = msg_queue.front();
    msg_queue.pop();

    return msg;
}

Server::ClientInfo::~ClientInfo(){ if(fd) close(fd);}