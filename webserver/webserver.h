#pragma once

#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>

#include "epoller.h"
#include "log.h"
#include "heaptimer.h"
#include "sqlconnpool.h"
#include "threadpool.h"
#include "sqlconnRAII.h"
#include "httpconnect.h"

class WebServer {
public:
    WebServer(
        int port, int trigMode, int timeoutMS, bool optLinger, //端口号，触发模式，超时时间，是否优雅退出
        int sqlPort, const char* sqlUser, const  char* sqlPwd, //mysql端口号、用户名、用户密码
        const char* dbName, int connPoolNum, int threadNum,    //数据库名，连接池数量，线程池数量
        bool openLog, int logLevel, int logQueSize);           //是否开启日志，日志等级，日志异步队列容量

    ~WebServer();
    void Start();

private:
    bool InitSocket_(); 
    void InitEventMode_(int trigMode);

    void DealListen_();
    void AddClient_(int fd, sockaddr_in addr);

    void SendError_(int fd, const char*info);
    void CloseConn_(HttpConnect* client);

    void DealRead_(HttpConnect* client);
    void OnRead_(HttpConnect* client);

    void DealWrite_(HttpConnect* client);
    void OnWrite_(HttpConnect* client);

    void OnProcess_(HttpConnect* client);
    
    
    void ExtentTime_(HttpConnect* client);
    static int SetFdNonblock_(int fd); //设置非阻塞

    static const int MAX_FD = 65536;  //最大连接数

    int port_;        //端口号
    bool openLinger_; //是否优雅退出
    int timeoutMS_;   //超时时间（毫秒）
    bool isClose_;    //是否关闭
    int listenFd_;    //监听socket连接的文件描述符
    char* srcDir_;    //网页文件目录
    
    uint32_t listenEvent_;  //监听的事件
    uint32_t connEvent_;    //连接的事件
   
    std::unique_ptr<HeapTimer> timer_;         //定时器
    std::unique_ptr<ThreadPool> threadpool_;   //线程池
    std::unique_ptr<Epoller> epoller_;         //epoll封装
    std::unordered_map<int, HttpConnect> users_;  //映射到客户连接
};

