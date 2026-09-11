#pragma once

#include <condition_variable>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>


// Мастерская с фиксированным числом кузнецов.
// Заказы кладут в очередь, свободный работник забирает следующий.
//
// Публичный интерфейс не меняй: на него завязаны кузница и тесты.
class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Положить заказ. Может вызываться из нескольких потоков сразу.
    void submit(std::function<void()> job);

    // Вернуться, когда очередь пуста и никто больше не куёт.
    void wait_idle();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> jobs;

    std::mutex m;
    std::condition_variable cv;

    int busy = 0;
    bool stopped = false;
    // Свои поля: воркеры, очередь задач, мьютекс, cv, флаг остановки...
};
