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
    std::thread::id worker_id;
    const auto main_id = std::this_thread::get_id();
    const auto t0 = std::chrono::steady_clock::now();

    const std::string answer = ask_oracle([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        worker_id = std::this_thread::get_id();
        return std::string{"42"};
    });

    const auto elapsed = std::chrono::steady_clock::now() - t0;

    CHECK(answer == "42");
    CHECK(worker_id != std::thread::id{});
    CHECK(worker_id != main_id);
    CHECK(elapsed >= std::chrono::milliseconds(80));

    std::cout << "OK  03_oracle: гонец сходил в другой поток и принёс \""
              << answer << "\"\n";
    return 0;
}
