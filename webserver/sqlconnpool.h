#pragma once

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "log.h"

class SqlConnPool {
public:
    static SqlConnPool *instance();

    void init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* db_name, int conn_size);
    
    MYSQL *get_connect();
    void free_connect(MYSQL * conn);
    int get_free_connect_count();

    void close_pool();

private:
    SqlConnPool(); //单例模式
    ~SqlConnPool();

    int max_connects_;
    int use_counts_;
    int free_counts_;

    std::queue<MYSQL *> connect_que_;
    std::mutex mtx_;
    sem_t sem_id_;
};

