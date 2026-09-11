#include "thread_pool.hpp"

#include <utility>

ThreadPool::ThreadPool(std::size_t worker_count) {
    if (worker_count == 0) {
        worker_count = 1;
    }
    workers_.reserve(worker_count);
    for (std::size_t i = 0; i < worker_count; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mu_);
        stop_ = true;
    }
    work_cv_.notify_all();
    for (std::thread& t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::submit(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (stop_) {
            return;
        }
        jobs_.push(std::move(job));
    }
    work_cv_.notify_one();
}

void ThreadPool::wait_idle() {
    std::unique_lock<std::mutex> lock(mu_);
    idle_cv_.wait(lock, [this] { return jobs_.empty() && busy_ == 0; });
}

void ThreadPool::worker_loop() {
    for (;;) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(mu_);
            work_cv_.wait(lock, [this] { return stop_ || !jobs_.empty(); });
            if (stop_ && jobs_.empty()) {
                return;
            }
            job = std::move(jobs_.front());
            jobs_.pop();
            ++busy_;
        }
        job();
        {
            std::lock_guard<std::mutex> lock(mu_);
            --busy_;
            if (jobs_.empty() && busy_ == 0) {
                idle_cv_.notify_all();
            }
        }
    }
}
