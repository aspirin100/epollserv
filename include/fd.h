#ifndef FD_H
#define FD_H

#include <unistd.h>

class FD
{
private:
    int fd_;

public:
    explicit FD(const int fd) : fd_(fd){}

    FD(const FD& other) = delete;
    FD& operator=(FD& rhs) = delete;

    FD(FD&& other)
    {
        other.fd_ = this->fd_;
        this->fd_ = 0;
    }

    FD& operator=(FD&& rhs)
    {
        rhs.fd_ = this->fd_;
        this->fd_ = 0;

        return *this;
    }

    ~FD(){if(fd_) close(fd_);}


public:

    int GetFD(){ return fd_; }
    void SetFD(const int fd){ fd_ = fd; }

    void Close()
    {
        if(fd_) close(fd_);
        fd_ = 0;
    }
};

#endif