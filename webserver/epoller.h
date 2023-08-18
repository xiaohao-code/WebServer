#pragma once

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024); //显式的构造

    ~Epoller();

    bool AddFd(int fd, uint32_t events);//添加要监视的文件描述符以及事件

    bool ModFd(int fd, uint32_t events);//修改fd上的事件

    bool DelFd(int fd);//删除监听的fd

    int Wait(int timeoutMs = -1);

    int GetEventFd(size_t i) const;

    uint32_t GetEvents(size_t i) const;
        
private:
    int epollFd_;//epoll池的文件描述符

    std::vector<struct epoll_event> events_;//存放返回的事件   
};
