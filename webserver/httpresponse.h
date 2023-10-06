#pragma once

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "buffer.h"
#include "log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& srcDir, std::string& path, bool is_keep_alive = false, int code = -1);
    void unmap_file();
    void make_response(Buffer& buff);
    
    char* file();
    size_t file_len() const;
    void error_content(Buffer& buff, std::string message);
    int code() const { return code_; }

private:
    void error_html();
    void add_state_line(Buffer &buff);
    void add_header(Buffer &buff);
    void add_content(Buffer &buff);
    std::string get_file_type();

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;

    int code_;
    bool is_keep_alive_;

    std::string path_;
    std::string src_dir_;
    
    char* mm_file_; 
    struct stat mm_file_stat_; //存放文件的元信息
};

