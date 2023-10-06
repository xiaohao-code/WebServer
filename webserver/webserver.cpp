#include "webserver.h"

using namespace std;

WebServer::WebServer(const ServerConfig &config)
    : port_(config.port),
      open_linger_(config.open_linger),
      timeout_ms_(config.timeout),
      is_close_(false),
      timer_(new HeapTimer()),
      threadpool_(new ThreadPool(config.thread_pool_num)),
      epoller_(new Epoller())
{
    src_dir_ = getcwd(nullptr, 256);    
    assert(src_dir_);                     
    strncat(src_dir_, "/resources/", 16);

    HttpConnect::user_count = 0;
    HttpConnect::src_dir = src_dir_;
    SqlConnPool::instance()->init("localhost", config.sql_port, config.sql_usr.c_str(), config.sql_pwd.c_str(), config.db_name.c_str(), config.sql_connect_num);

    if(config.open_log) {
        Log::instance()->init(config.log_level, "./log", ".log", config.log_que_size);
        if(is_close_) { 
            LOG_ERROR("========== Server init error!=========="); 
        } else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, open_linger_ ? "true":"false");
            LOG_INFO("LogSys level: %d", config.log_level);
            LOG_INFO("srcDir: %s", HttpConnect::src_dir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", config.sql_connect_num, config.thread_pool_num);
        }
    }

    init_event_mode(config.trig_mode);
    if(!init_socket()) { 
        is_close_ = true;
    }
}

WebServer::WebServer(int port, int trig_mode, int timeout_ms, bool opt_linger,
                        int sql_port, const char* sql_user, const  char* sql_pwd,
                        const char* db_name, int connect_pool_num, int thread_num,
                        bool open_log, int log_level, int log_que_size)
    : port_(port), 
      open_linger_(opt_linger), 
      timeout_ms_(timeout_ms), 
      is_close_(false),
      timer_(new HeapTimer()), 
      threadpool_(new ThreadPool(thread_num)), 
      epoller_(new Epoller()) 
{
    src_dir_ = getcwd(nullptr, 256);    
    assert(src_dir_);                     
    strncat(src_dir_, "/resources/", 16);

    HttpConnect::user_count = 0;             
    HttpConnect::src_dir = src_dir_;           
    SqlConnPool::instance()->init("localhost", sql_port, sql_user, sql_pwd, db_name, connect_pool_num);

    if(open_log) {
        Log::instance()->init(log_level, "./log", ".log", log_que_size);
        if(is_close_) { 
            LOG_ERROR("========== Server init error!=========="); 
        } else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, opt_linger ? "true":"false");
            LOG_INFO("LogSys level: %d", log_level);
            LOG_INFO("srcDir: %s", HttpConnect::src_dir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connect_pool_num, thread_num);
        }
    }

    init_event_mode(trig_mode);
    if(!init_socket()) { 
        is_close_ = true;
    }
}

WebServer::~WebServer() {
    close(listen_fd_);
    is_close_ = true;
    free(src_dir_);
    SqlConnPool::instance()->close_pool();
}

void WebServer::start_server() {
    int time_ms = -1;
    if(!is_close_) { 
        LOG_INFO("========== Server start =========="); 
    }
    while(!is_close_) {
        if(timeout_ms_ > 0) {
            time_ms = timer_->get_next_tick(); // 获得离最近的超时点还剩多长时间
        }
        int event_count = epoller_->wait(time_ms);
        for(int i = 0; i < event_count; i++) {
            int fd = epoller_->get_event_fd(i);
            uint32_t events = epoller_->get_events(i);
            if(fd == listen_fd_) {  // 有新的客户端连接进来
                deal_listen();
            } else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {  //客户端断开连接
                assert(users_.count(fd) > 0);
                close_connect(&users_[fd]);
            } else if(events & EPOLLIN) {  //有需要读的数据
                assert(users_.count(fd) > 0);
                deal_read(&users_[fd]);
            } else if(events & EPOLLOUT) {  //有需要写的数据
                assert(users_.count(fd) > 0);
                deal_write(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

//创建套接字，获得监听的文件描述符
bool WebServer::init_socket() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }
    addr.sin_family = AF_INET;  //IPV4
    addr.sin_addr.s_addr = htonl(INADDR_ANY);// 绑定本机任何IP地址，转成网络字节序
    addr.sin_port = htons(port_);//绑定端口
    struct linger opt_linger = { 0 };
    if(open_linger_) {
        // 优雅关闭: 直到所剩数据发送完毕或超时 
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);// 创建TCP套接字的监听描述符
    if(listen_fd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    // 设置套接字是否打开优雅关闭
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
    if(ret < 0) {
        close(listen_fd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    // 端口复用,只有最后一个套接字会正常接收数据 
    int opt_val = 1;
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt_val, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listen_fd_);
        return false;
    }

    //绑定地址和端口
    ret = bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listen_fd_);
        return false;
    }

    ret = listen(listen_fd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listen_fd_);
        return false;
    }
    ret = epoller_->add_fd(listen_fd_,  listen_event_ | EPOLLIN);// 将套接字对应的fd添加到epoll中
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listen_fd_);
        return false;
    }
    set_fd_nonblock(listen_fd_);// 设置非阻塞
    LOG_INFO("Server port:%d", port_);
    return true;
}

//初始化事件监听模式和触发模式
void WebServer::init_event_mode(int trig_mode) {
    listen_event_ = EPOLLRDHUP;    //监听事件为是否被挂断
    connect_event_ = EPOLLONESHOT | EPOLLRDHUP; //只监听一次，监听是否被挂断
    switch (trig_mode) {        //设置触发模式
    case 0:
        break;
    case 1:
        connect_event_ |= EPOLLET;  
        break;
    case 2:
        listen_event_ |= EPOLLET; 
        break;
    case 3:
        listen_event_ |= EPOLLET;  
        connect_event_ |= EPOLLET;
        break;
    default:
        listen_event_ |= EPOLLET;
        connect_event_ |= EPOLLET;
        break;
    }
    HttpConnect::is_ET = (connect_event_ & EPOLLET);
    LOG_INFO("Listen Mode: %s, OpenConn Mode: %s", 
                (listen_event_ & EPOLLET ? "ET" : "LT"), 
                (connect_event_ & EPOLLET ? "ET" : "LT"));
}

//处理新的客户端连接
void WebServer::deal_listen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listen_fd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { 
            return;
        } else if(HttpConnect::user_count >= MAX_FD) {
            send_error(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        add_client(fd, addr);
    } while(listen_event_ & EPOLLET);
}

void WebServer::send_error(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

// 添加新的客户端连接
void WebServer::add_client(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if(timeout_ms_ > 0) {
        timer_->add(fd, timeout_ms_, std::bind(&WebServer::close_connect, this, &users_[fd]));// 如果定时时间到则断开连接
    }
    epoller_->add_fd(fd, EPOLLIN | connect_event_);// 监听新的连接的事件
    set_fd_nonblock(fd);// 设置非阻塞
    LOG_INFO("Client[%d] in!", users_[fd].get_fd());
}

// 断开连接
void WebServer::close_connect(HttpConnect* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->get_fd());
    epoller_->del_fd(client->get_fd());
    client->close_connect();
}

//读取客户端请求
void WebServer::deal_read(HttpConnect* client) {
    assert(client);
    extent_time(client);
    threadpool_->add_task(std::bind(&WebServer::on_read, this, client));
}

void WebServer::on_read(HttpConnect* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read_request(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        close_connect(client);
        return;
    }
    on_process(client);
}

// 将响应发送给客户端
void WebServer::deal_write(HttpConnect* client) {
    assert(client);
    extent_time(client);
    threadpool_->add_task(std::bind(&WebServer::on_write, this, client));
}

void WebServer::on_write(HttpConnect* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write_response(&writeErrno);
    if(client->to_write_bytes() == 0) {
        // 传输完成
        if(client->is_keep_alive()) {
            on_process(client);
            return;
        }
    } else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            // 继续传输 
            epoller_->mod_fd(client->get_fd(), connect_event_ | EPOLLOUT);
            return;
        }
    }
    close_connect(client);
}

//处理客户端请求
void WebServer::on_process(HttpConnect* client) {
    if(client->process()) {
        epoller_->mod_fd(client->get_fd(), connect_event_ | EPOLLOUT); //读完之后更改fd的事件类型
    } else {
        epoller_->mod_fd(client->get_fd(), connect_event_ | EPOLLIN);
    }
}

//因为有新操作，所以延长定时器时间
void WebServer::extent_time(HttpConnect* client) {
    assert(client);
    if(timeout_ms_ > 0) { 
        timer_->adjust(client->get_fd(), timeout_ms_); 
    }
}

//设置为非阻塞
int WebServer::set_fd_nonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


