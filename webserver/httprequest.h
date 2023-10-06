//处理以及存储HTTP请求的内容

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  

#include "buffer.h"
#include "log.h"
#include "sqlconnpool.h"
#include "sqlconnRAII.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    HttpRequest() { init(); }
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string get_post(const std::string& key) const;
    std::string get_post(const char* key) const;
    bool is_keep_alive() const;

    /* 
    todo 
    void HttpConnect::ParseFormData() {}
    void HttpConnect::ParseJson() {}
    */

private:
    static int conver_hex(char ch);
    bool parse_request_line(const std::string& line);
    void parse_path();
    void parse_header(const std::string& line);
    void parse_body(const std::string& line);
    void parse_post();
    void parse_from_urlencoded();

    static bool user_verify(const std::string& name, const std::string& pwd, bool is_login);

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    PARSE_STATE state_; //表明解析请求报文的位置
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_; //请求头的内容
    std::unordered_map<std::string, std::string> post_;   //表单数据内容

    
    
};

