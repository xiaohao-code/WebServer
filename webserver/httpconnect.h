//HTTP客户端连接对应的对象

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

    void init(int sockFd, const sockaddr_in& addr);

    ssize_t read_request(int* saveErrno);

    bool process();

    ssize_t write_response(int* saveErrno);

    void close_connect();

    int get_fd() const;

    int get_port() const;

    const char* get_ip() const;
    
    sockaddr_in get_addr() const;

    int to_write_bytes() const;

    bool is_keep_alive() const;

    static bool is_ET;
    static const char* src_dir;
    static std::atomic<int> user_count; //原子操作,类的静态变量
    
private:
    int fd_;                      //对应socket的fd
    struct sockaddr_in addr_;     //对应客户端的地址

    bool is_close_;
    
    int iov_count_;
    struct iovec iov_[2];
    
    Buffer read_buff_;             //读缓冲区
    Buffer write_buff_;            //写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};

