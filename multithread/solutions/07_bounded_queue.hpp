#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : cap_(capacity == 0 ? 1 : capacity) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(mu_);
        not_full_.wait(lock, [this] { return q_.size() < cap_ || closed_; });
        if (closed_) {
            return;
        }
        q_.push(std::move(item));
        not_empty_.notify_one();
    }

    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mu_);
        not_empty_.wait(lock, [this] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return false;
        }
        out = std::move(q_.front());
        q_.pop();
        not_full_.notify_one();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::size_t cap_;
    std::queue<T> q_;
    std::mutex mu_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    bool closed_ = false;
};
