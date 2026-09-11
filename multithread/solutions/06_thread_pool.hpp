#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> job);
    void wait_idle();

private:
    void worker_loop();

    std::mutex mu_;
    std::condition_variable work_cv_;
    std::condition_variable idle_cv_;
    std::queue<std::function<void()>> jobs_;
    std::vector<std::thread> workers_;
    std::size_t busy_ = 0;
    bool stop_ = false;
};
