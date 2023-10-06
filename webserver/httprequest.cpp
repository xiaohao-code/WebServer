
#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture", 
};

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0}, 
    {"/login.html", 1},  
};

void HttpRequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.readable_bytes() <= 0) {
        return false;
    }
    while(buff.readable_bytes() && state_ != FINISH) {
        const char* lineEnd = search(buff.peek(), buff.begin_write_const(), CRLF, CRLF + 2); //以\r\n分隔每一行
        std::string line(buff.peek(), lineEnd); 
        switch(state_)
        {
        case REQUEST_LINE:
            if(!parse_request_line(line)) {
                return false;
            }
            parse_path();
            break;    
        case HEADERS:
            parse_header(line);
            if(buff.readable_bytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            parse_body(line);
            break;
        default:
            break;
        }
        if(lineEnd == buff.begin_write()) { 
            break; 
        }
        buff.retrieve_until(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::get_post(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::get_post(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

bool HttpRequest::is_keep_alive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

int HttpRequest::conver_hex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

//解析请求行
bool HttpRequest::parse_request_line(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

//解析请求的html网页文件
void HttpRequest::parse_path() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    } else {
        for(auto &item : DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

//解析请求头
void HttpRequest::parse_header(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = BODY;
    }
}

//解析请求体
void HttpRequest::parse_body(const string& line) {
    body_ = line;
    parse_post();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::parse_post() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        parse_from_urlencoded();
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool is_login = (tag == 1);
                if(user_verify(post_["username"], post_["password"], is_login)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }   
}

void HttpRequest::parse_from_urlencoded() {
    if(body_.size() == 0) { 
        return; 
    }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = conver_hex(body_[i + 1]) * 16 + conver_hex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::user_verify(const string &name, const string &pwd, bool is_login) {
    if(name == "" || pwd == "") { 
        return false; 
    }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::instance());
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    if(!is_login) { 
        flag = true; 
    }
    // 查询用户及密码
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        //登陆行为
        if(is_login) {
            if(pwd == password) { 
                flag = true; 
            } else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } else { 
            flag = false; 
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    // 注册行为且用户名未被使用
    if(!is_login && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    SqlConnPool::instance()->free_connect(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}