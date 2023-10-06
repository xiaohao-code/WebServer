#include <string>
#include <unistd.h>
#include <string.h>

class ServerConfig {
public:
    ServerConfig() = default;

    void set_server_port(int port);

    void set_open_linger(int opt_linger);

    void set_timeout(int timeout_ms);

    void set_resource_dir(const std::string &dir);

    void set_sql_port(int port);

    void set_sql_user(const std::string &user);

    void set_sql_pwd(const std::string &pwd);

    void set_sql_connect_num(int num);

    void set_log_level(int level);

    void set_thread_num(int num);

    int port = 1314;               // 端口号,默认是1314
    int trig_mode = 3;              // 事件触发模式ET/LT
    bool open_linger = false;      // 是否优雅退出
    int timeout = 60000;           // 超时时间（毫秒）
    char *resource_dir;            // 网页文件夹目录

    int sql_port = 3306;                  // sql端口
    std::string sql_usr = "root";         // sql用户名
    std::string sql_pwd = "123456";       // sql密码
    std::string db_name = "webserver";     // 数据库名
    int sql_connect_num = 12;

    bool open_log = true;            // 日志开关
    int log_level = 1;               // 日志等级
    int log_que_size = 1024;          // 日志队列长度

    int thread_pool_num = 6;
};