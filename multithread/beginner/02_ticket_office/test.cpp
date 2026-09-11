#include "task.hpp"

#include <iostream>
#include <thread>
#include <vector>

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::cerr << "FAIL: " #cond << "  (" << __FILE__ << ":"          \
                      << __LINE__ << ")\n";                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main() {
    constexpr int kWindows = 8;
    constexpr int kPerWindow = 100000;
    constexpr int kExpected = kWindows * kPerWindow;

    int tickets_sold = 0;
    std::mutex cash_register;
    std::vector<std::thread> cashiers;
    cashiers.reserve(kWindows);

    for (int i = 0; i < kWindows; ++i) {
        cashiers.emplace_back([&] {
            for (int n = 0; n < kPerWindow; ++n) {
                sell_ticket(tickets_sold, cash_register);
            }
        });
    }
    for (auto& t : cashiers) {
        t.join();
    }

    CHECK(tickets_sold == kExpected);

    std::cout << "OK  02_ticket_office: продано " << tickets_sold
              << " билетов, ни один не потерялся\n";
    return 0;
}
