
#include "log.h"

using namespace std;

Log::Log() {
    line_count_ = 0;
    is_async_ = false;
    write_thread_ = nullptr;
    deque_ = nullptr;
    today_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if(write_thread_ && write_thread_->joinable()) {
        while(!deque_->empty()) {
            deque_->flush();
        };
        deque_->close();
        write_thread_->join();
    }
    if(fp_) {
        lock_guard<mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

Log* Log::instance() {
    static Log inst;
    return &inst;
}

void Log::init(int level = 1, const char* path, const char* suffix, int max_size) {
    is_open_ = true;
    level_ = level;
    if(max_size > 0) {
        is_async_ = true;
        if(!deque_) {
            unique_ptr<BlockDeque<std::string>> new_deque(new BlockDeque<std::string>);
            deque_ = move(new_deque);
            
            std::unique_ptr<std::thread> new_thread(new thread(flush_log_thread));
            write_thread_ = move(new_thread);
        }
    } else {
        is_async_ = false;
    }

    line_count_ = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    today_ = t.tm_mday;

    {
        lock_guard<mutex> locker(mtx_);
        buff_.retrieve_all();
        if(fp_) { 
            flush();
            fclose(fp_); 
        }

        fp_ = fopen(fileName, "a");
        if(fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        } 
        assert(fp_ != nullptr);
    }
}

//将日志内容添加到队列中
void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);//获取当前时间(秒数)
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);//获取当前时间(年月日时分秒)
    struct tm t = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    if (today_ != t.tm_mday || (line_count_ && (line_count_  %  MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(mtx_);
        locker.unlock();
        
        char new_file[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);//将年月日格式化放入tail中

        if (today_ != t.tm_mday)
        {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            today_ = t.tm_mday;
            line_count_ = 0;
        } else {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (line_count_ / MAX_LINES), suffix_); //处理日志数量过多的情况
        }
        
        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(new_file, "a");
        assert(fp_ != nullptr);
    }

    {
        unique_lock<mutex> locker(mtx_);
        ++line_count_;
        int n = snprintf(buff_.begin_write(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        buff_.has_written(n);
        append_log_level_title(level);

        va_start(vaList, format);
        int m = vsnprintf(buff_.begin_write(), buff_.writable_bytes(), format, vaList);
        va_end(vaList);

        buff_.has_written(m);
        buff_.append("\n\0", 2);

        if(is_async_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.retrieve_all_to_str());
        } else {
            fputs(buff_.peek(), fp_);
        }
        buff_.retrieve_all();
    }
}

int Log::get_level() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::set_level(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}

void Log::append_log_level_title(int level) {
    switch(level) {
    case 0:
        buff_.append("[debug]: ", 9);
        break;
    case 1:
        buff_.append("[info] : ", 9);
        break;
    case 2:
        buff_.append("[warn] : ", 9);
        break;
    case 3:
        buff_.append("[error]: ", 9);
        break;
    default:
        buff_.append("[info] : ", 9);
        break;
    }
}

void Log::flush() {
    if(is_async_) { 
        deque_->flush(); 
    }
    fflush(fp_);
}

void Log::flush_log_thread() {
    Log::instance()->async_write();
}

void Log::async_write() {
    string str = "";
    while(deque_->pop(str)) {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

