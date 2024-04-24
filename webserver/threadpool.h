#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count = 8) : pool_(std::make_shared<Pool>()) {
        assert(thread_count > 0);
        for(size_t i = 0; i < thread_count; i++) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> locker(pool->mtx);
                while(true) {
                    if(!pool->tasks.empty()) {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    } else if (pool->is_closed) {
                        break;
                    } else {
                        pool->cond.wait(locker);
                    }
                }
            }).detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->is_closed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void add_task(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_closed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};

