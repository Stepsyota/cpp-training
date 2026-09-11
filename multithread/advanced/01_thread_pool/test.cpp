#include "smithy.hpp"
#include "thread_pool.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace {

template <typename F>
bool finishes_within(F&& fn, std::chrono::milliseconds limit) {
    std::promise<void> done;
    std::future<void> ready = done.get_future();
    std::thread runner([&] {
        fn();
        done.set_value();
    });
    if (ready.wait_for(limit) != std::future_status::ready) {
        runner.detach();
        return false;
    }
    runner.join();
    return true;
}

}  // namespace

int main() {
    using namespace std::chrono_literals;

    {
        std::atomic<int> completed{0};
        {
            ThreadPool pool(3);
            run_shift(pool,
                      std::vector<Order>{{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}},
                      completed);
            if (!finishes_within([&] { pool.wait_idle(); }, 2s)) {
                std::cerr << "FAIL: wait_idle завис (забыли notify / счётчик занятых?)\n";
                return 1;
            }
        }
        if (completed.load() != 5) {
            std::cerr << "FAIL: выполнено " << completed.load() << " заказов из 5\n";
            return 1;
        }
    }

    {
        std::mutex mu;
        std::set<std::thread::id> ids;
        const auto main_id = std::this_thread::get_id();
        {
            ThreadPool pool(3);
            for (int i = 0; i < 12; ++i) {
                pool.submit([&] {
                    std::this_thread::sleep_for(5ms);
                    std::lock_guard<std::mutex> lock(mu);
                    ids.insert(std::this_thread::get_id());
                });
            }
            if (!finishes_within([&] { pool.wait_idle(); }, 2s)) {
                std::cerr << "FAIL: wait_idle завис на проверке потоков\n";
                return 1;
            }
        }
        if (ids.empty()) {
            std::cerr << "FAIL: ни один заказ не выполнен\n";
            return 1;
        }
        if (ids.size() == 1 && *ids.begin() == main_id) {
            std::cerr << "FAIL: работа ушла в вызывающий поток — submit не должен "
                         "вызывать job() сам\n";
            return 1;
        }
        if (ids.size() < 2) {
            std::cerr << "FAIL: все заказы сделал один поток, ждали пул из трёх\n";
            return 1;
        }
    }

    {
        const auto t0 = std::chrono::steady_clock::now();
        {
            ThreadPool pool(4);
            std::atomic<int> completed{0};
            std::vector<Order> batch(4, Order{0, 80});
            run_shift(pool, batch, completed);
            if (!finishes_within([&] { pool.wait_idle(); }, 2s)) {
                std::cerr << "FAIL: параллельные заказы не дождались\n";
                return 1;
            }
            if (completed.load() != 4) {
                std::cerr << "FAIL: параллельно доковано " << completed.load()
                          << " из 4\n";
                return 1;
            }
        }
        const auto elapsed = std::chrono::steady_clock::now() - t0;
        if (elapsed > 250ms) {
            std::cerr << "FAIL: 4 заказа по 80мс на 4 кузнецах заняли слишком "
                         "долго — похоже, они идут по одному\n";
            return 1;
        }
    }

    std::cout << "OK  06_thread_pool: кузнецы разобрали очередь, пул не "
                 "убил процесс на выходе\n";
    return 0;
}
