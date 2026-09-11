#include "thread_pool.hpp"
#include <iostream>
#include <exception>


ThreadPool::ThreadPool(std::size_t worker_count) {
    // TODO: подними worker_count потоков. Каждый в цикле:
    //   ждёт задачу на condition_variable, берёт из очереди, выполняет.
    // worker_count == 0 считай за 1.

    if (worker_count == 0) worker_count = 1;

    for (std::size_t i = 0; i < worker_count; ++i) {
        workers.emplace_back(
            [this] {
                while(true){
                    std::function<void()> job;
                    {
                        std::unique_lock<std::mutex> lock(m);
                        cv.wait(lock, [this] {return !jobs.empty() || stopped;});

                        if (jobs.empty() && stopped) {
                            return;
                        }
    
                        job = jobs.front();
                        jobs.pop();
    
                        ++busy;
                    }
                    try {
                        job();
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Error in job: " << e.what() << '\n'; 
                    }
                    {
                        std::lock_guard<std::mutex> lock(m);
                        --busy;

                        if (jobs.empty() && (busy == 0)) {
                            cv.notify_all();
                        }
                    }
                }
            }
        );
    }
}

ThreadPool::~ThreadPool() { 
    // TODO: запрети новую работу, разбуди всех, join каждого кузнеца.
    // Иначе деструктор ручки std::thread вызовет terminate.

    {
        std::lock_guard<std::mutex> lock(m);
        stopped = true;
    }
    cv.notify_all();

    for (auto& worker: workers) {
        worker.join();
    }
}

void ThreadPool::submit(std::function<void()> job) {
    // TODO: положи job в очередь и разбуди одного спящего.
    // Не вызывай job() здесь — тогда заказ уйдёт в поток вызывающего.
    {
        std::lock_guard<std::mutex> lock(m);

        if (stopped) {
            return;
        }

        jobs.emplace(job);
    }
    cv.notify_one();
}

void ThreadPool::wait_idle() {
    // TODO: спи, пока очередь не пуста и ни один работник не занят.
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [this] {return jobs.empty() && (busy == 0);});
}
