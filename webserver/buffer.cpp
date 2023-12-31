
#include "buffer.h"

Buffer::Buffer(int init_buff_size) : 
    buffer_(init_buff_size), read_pos_(0), write_pos_(0) {}

size_t Buffer::readable_bytes() const {
    return write_pos_ - read_pos_;
}

size_t Buffer::writable_bytes() const {
    return buffer_.size() - write_pos_;
}

//返回指向下一个可读字符的指针
const char* Buffer::peek() const {
    return begin_ptr() + read_pos_;
}

void Buffer::ensure_writeable(size_t len) {
    if(writable_bytes() < len) {
        make_space(len);
    }
    assert(writable_bytes() >= len);
}

void Buffer::has_written(size_t len) {
    write_pos_ += len;
}

//在前部恢复一定长度的空间
void Buffer::retrieve(size_t len) {
    assert(len <= readable_bytes());
    read_pos_ += len;
}

void Buffer::retrieve_until(const char* end) {
    assert(peek() <= end );
    retrieve(end - peek());
}

//清空缓存区内容
void Buffer::retrieve_all() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}

//将可读内容转换为string并清空缓存区内容
std::string Buffer::retrieve_all_to_str() {
    std::string str(peek(), readable_bytes());
    retrieve_all();
    return str;
}

const char* Buffer::begin_write_const() const {
    return begin_ptr() + write_pos_;
}

char* Buffer::begin_write() {
    return begin_ptr() + write_pos_;
}                                             

void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

void Buffer::append(const void* data, size_t len) {
    assert(data);
    append(static_cast<const char*>(data), len);
}

void Buffer::append(const char* str, size_t len) {
    assert(str);
    ensure_writeable(len);
    std::copy(str, str + len, begin_write());
    has_written(len);
}

void Buffer::append(const Buffer& buff) {
    append(buff.peek(), buff.readable_bytes());
}

ssize_t Buffer::read_fd(int fd, int* errno_flag) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = writable_bytes();
    //分散读， 保证数据全部读完 
    iov[0].iov_base = begin_ptr() + write_pos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *errno_flag = errno;
    } else if(static_cast<size_t>(len) <= writable) {
        write_pos_ += len;
    } else {
        write_pos_ = buffer_.size();
        append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::write_fd(int fd, int* errno_flag) {
    size_t readable = readable_bytes();
    ssize_t len = write(fd, peek(), readable);
    if(len < 0) {
        *errno_flag = errno;
        return len;
    } 
    read_pos_ += len;
    return len;
}

char* Buffer::begin_ptr() {
    return &*buffer_.begin();
}

const char* Buffer::begin_ptr() const {
    return &*buffer_.begin();
}

//得到已经读取过的字节数，为了后续可以使用前面已经读过的空间
size_t Buffer::has_read_bytes() const {
    return read_pos_;
}

void Buffer::make_space(size_t len) {
    if(writable_bytes() + has_read_bytes() < len) {
        buffer_.resize(write_pos_ + len + 1);
    } else {
        size_t readable = readable_bytes();
        std::copy(begin_ptr() + read_pos_, begin_ptr() + write_pos_, begin_ptr());//将缓冲区拷贝到开头
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable;
        assert(readable == readable_bytes());
    }
}