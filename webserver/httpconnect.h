#pragma once

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "log.h"
#include "sqlconnRAII.h"
#include "buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConnect {
public:
    HttpConnect();

    ~HttpConnect();

    void Init(int sockFd, const sockaddr_in& addr);

    ssize_t Read(int* saveErrno);

    ssize_t Write(int* saveErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char* GetIP() const;
    
    sockaddr_in GetAddr() const;
    
    bool Process();

//需要写入到socket的字节数
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount; //原子操作
    
private:
    int fd_;                      //对应socket的fd
    struct sockaddr_in addr_;     //对应客户端的地址

    bool isClose_;
    
    int iovCnt_;
    struct iovec iov_[2];
    
    Buffer readBuff_;             //读缓冲区
    Buffer writeBuff_;            //写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};

