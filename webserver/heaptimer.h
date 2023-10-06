//通过小根堆来实现定时器功能

#pragma once

#include <time.h>
#include <arpa/inet.h> 
#include <assert.h>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <functional> 
#include <chrono>
#include "log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }

    ~HeapTimer() { clear(); }

    void clear();

    void add(int id, int timeOut, const TimeoutCallBack& cb);
    
    void adjust(int id, int newExpires);

    void doWork(int id);

    void tick();

    void pop();

    int get_next_tick();

private:
    void del(size_t i);
    
    void siftup(size_t i);

    bool siftdown(size_t index, size_t n);

    void swap_node(size_t i, size_t j);

    std::vector<TimerNode> heap_;

    std::unordered_map<int, size_t> ref_; //fd与小根堆中id的对应
};
