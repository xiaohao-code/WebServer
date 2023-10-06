#include "serverconfig.h"


void ServerConfig::set_server_port(int p) {
    port = p;
}

void ServerConfig::set_open_linger(int opt_linger) {
    if (opt_linger > 0) {
        open_linger = true;
    } else {
        open_linger = false;
    }
}

void ServerConfig::set_timeout(int timeout_ms) {
    timeout = timeout_ms;
}

void ServerConfig::set_resource_dir(const std::string &dir) {
    resource_dir = getcwd(nullptr, 256);
    strncat(resource_dir, "/resources/", 16);
}

void ServerConfig::set_sql_port(int p) {
    port = p;
}

void ServerConfig::set_sql_user(const std::string &usr) {
    sql_usr = usr;
}

void ServerConfig::set_sql_pwd(const std::string &pwd) {
    sql_pwd = pwd;
}

void ServerConfig::set_sql_connect_num(int num) {
    sql_connect_num = num;
}

void ServerConfig::set_log_level(int level) {
    log_level = level;
}

void ServerConfig::set_thread_num(int num) {
    thread_pool_num = num;
}
