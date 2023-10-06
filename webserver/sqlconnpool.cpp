
#include "sqlconnpool.h"
using namespace std;

SqlConnPool::SqlConnPool() {
    use_counts_ = 0;
    free_counts_ = 0;
}

SqlConnPool::~SqlConnPool() {
    close_pool();
}

//单例模式，实例化数据库连接池
SqlConnPool* SqlConnPool::instance() {
    static SqlConnPool conn_pool;
    return &conn_pool;
}

//初始化连接池
void SqlConnPool::init(const char* host, int port,
            const char* user,const char* pwd, const char* dbName,
            int conn_size = 10) {
    assert(conn_size > 0);
    for (int i = 0; i < conn_size; i++) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!");
        }
        connect_que_.push(sql);
    }
    max_connects_ = conn_size;
    sem_init(&sem_id_, 0, max_connects_);
}

//得到一个通向数据库地连接
MYSQL* SqlConnPool::get_connect() {
    MYSQL *sql = nullptr;
    if(connect_que_.empty()){
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&sem_id_);
    {
        lock_guard<mutex> locker(mtx_);
        sql = connect_que_.front();
        connect_que_.pop();
    }
    return sql;
}

//使用完的通向数据库的连接重新加入到队列中
void SqlConnPool::free_connect(MYSQL* sql) {
    assert(sql);
    lock_guard<mutex> locker(mtx_);
    connect_que_.push(sql);
    sem_post(&sem_id_);
}

//关闭数据库
void SqlConnPool::close_pool() {
    lock_guard<mutex> locker(mtx_);
    while(!connect_que_.empty()) {
        auto item = connect_que_.front();
        connect_que_.pop();
        mysql_close(item);
    }
    mysql_library_end();        
}

//获取目前可用的通向数据库的连接数量
int SqlConnPool::get_free_connect_count() {
    lock_guard<mutex> locker(mtx_);
    return connect_que_.size();
}


