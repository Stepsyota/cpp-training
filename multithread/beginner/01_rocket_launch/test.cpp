#include "task.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::cerr << "FAIL: " #cond << "  (" << __FILE__ << ":"          \
                      << __LINE__ << ")\n";                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main() {
    bool started = false;
    bool finished = false;
    std::thread::id worker_id;

    const auto main_id = std::this_thread::get_id();
    const auto t0 = std::chrono::steady_clock::now();

    run_prelaunch_check([&] {
        started = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        worker_id = std::this_thread::get_id();
        finished = true;
    });

    const auto elapsed = std::chrono::steady_clock::now() - t0;

    CHECK(started);
    CHECK(finished);
    CHECK(worker_id != main_id);
    CHECK(elapsed >= std::chrono::milliseconds(80));

    std::cout << "OK  01_rocket_launch: проверка ушла в другой поток, "
                 "пуск дождался её окончания\n";
    return 0;
}
