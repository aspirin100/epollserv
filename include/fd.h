#ifndef FD_H
#define FD_H

#include <unistd.h>

// TODO: handle standart fds(0, 1, 2)
struct FD
{
    int fd;
    ~FD(){if(fd > 0) close(fd);}
};

#endif