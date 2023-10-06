
#include "httpconnect.h"

using namespace std;

bool HttpConnect::is_ET;
const char* HttpConnect::src_dir;
std::atomic<int> HttpConnect::user_count;

HttpConnect::HttpConnect() { 
    fd_ = -1;
    addr_ = { 0 };
    is_close_ = true;
};

HttpConnect::~HttpConnect() { 
    close_connect(); 
};

void HttpConnect::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    user_count++;
    addr_ = addr;
    fd_ = fd;
    write_buff_.retrieve_all(); //清空读缓存区
    read_buff_.retrieve_all();  //清空写缓存区
    is_close_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, get_ip(), get_port(), (int)user_count);
}

//读入客户端请求至读缓存
ssize_t HttpConnect::read_request(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = read_buff_.read_fd(fd_, saveErrno);
        if (len <= 0) {  //读完
            break;
        }
    } while (is_ET);
    return len;
}

//处理请求，解析请求并初始化响应，将响应的内容存在写缓存中
bool HttpConnect::process() {
    request_.init();
    if(read_buff_.readable_bytes() <= 0) {
        return false;
    }
    else if(request_.parse(read_buff_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.init(src_dir, request_.path(), request_.is_keep_alive(), 200);
    } else {
        response_.init(src_dir, request_.path(), false, 400);
    }

    //相应行和响应头
    response_.make_response(write_buff_);
    iov_[0].iov_base = const_cast<char*>(write_buff_.peek());
    iov_[0].iov_len = write_buff_.readable_bytes();
    iov_count_ = 1;

    // 响应体直接使用内存中映射的文件
    if(response_.file_len() > 0  && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.file_len();
        iov_count_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.file_len() , iov_count_, to_write_bytes());
    return true;
}

//将写缓存发送给客户端
ssize_t HttpConnect::write_response(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_count_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { 
            break; 
        } else if(static_cast<size_t>(len) > iov_[0].iov_len) {  //iov_[1]中还有数据未传完
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                write_buff_.retrieve_all();
                iov_[0].iov_len = 0;
            }
        } else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            write_buff_.retrieve(len);
        }
    } while(is_ET || to_write_bytes() > 10240);
    return len;
}

void HttpConnect::close_connect() {
    response_.unmap_file();
    if(is_close_ == false) {
        is_close_ = true; 
        --user_count;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, get_ip(), get_port(), (int)user_count);
    }
}

int HttpConnect::get_fd() const {
    return fd_;
};

int HttpConnect::get_port() const {
    return addr_.sin_port;
}

const char* HttpConnect::get_ip() const {
    return inet_ntoa(addr_.sin_addr);
}

struct sockaddr_in HttpConnect::get_addr() const {
    return addr_;
}

int HttpConnect::to_write_bytes() const {
    return iov_[0].iov_len + iov_[1].iov_len; 
}

bool HttpConnect::is_keep_alive() const {
    return request_.is_keep_alive();
}









