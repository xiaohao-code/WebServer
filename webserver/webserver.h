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
#include "serverconfig.h"

class WebServer {
public:

    WebServer(const ServerConfig &config);

    WebServer(
        int port, int trig_mode, int timeout_ms, bool opt_linger,   //端口号，触发模式，超时时间，是否优雅退出
        int sql_port, const char* sql_user, const char* sql_pwd,    //mysql端口号、用户名、用户密码
        const char* db_name, int connect_pool_num, int thread_num,  //数据库名，数据库连接池数量，线程池数量
        bool open_log, int log_level, int log_que_size);            //是否开启日志，日志等级，日志异步队列容量

    ~WebServer();
    void start_server();

private:
    bool init_socket(); 

    void init_event_mode(int trig_mode);

    void deal_listen();

    void add_client(int fd, sockaddr_in addr);

    void send_error(int fd, const char*info);

    void close_connect(HttpConnect* client);

    void deal_read(HttpConnect* client);

    void on_read(HttpConnect* client);

    void deal_write(HttpConnect* client);

    void on_write(HttpConnect* client);

    void on_process(HttpConnect* client);
    
    void extent_time(HttpConnect* client);

    static int set_fd_nonblock(int fd);

    static const int MAX_FD = 65536; 
    int port_;        
    bool open_linger_; 
    int timeout_ms_;   
    bool is_close_;    
    int listen_fd_;    
    char* src_dir_;    
    uint32_t listen_event_;  
    uint32_t connect_event_;    
    std::unique_ptr<HeapTimer> timer_;            
    std::unique_ptr<ThreadPool> threadpool_;      
    std::unique_ptr<Epoller> epoller_;            
    std::unordered_map<int, HttpConnect> users_;  
};

