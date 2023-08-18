#include "webserver.h"

int main()
{

    WebServer server(
        1316, 3, 60000, false,                // 端口 ET模式 timeoutMs 优雅退出
        3306, "root", "123456", "webserver",  // Mysql配置:端口 用户名 用户密码 数据库名
        12, 6, true, 1, 1024);                // 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量
    
    server.Start();
} 
  