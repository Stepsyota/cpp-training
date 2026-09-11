#pragma once

#include <chrono>
#include <thread>

// Заказ из соседнего цеха. Не меняй.
struct Order {
    int id = 0;
    int hardness_ms = 0;
};

inline void fulfill(const Order& order) {
    if (order.hardness_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(order.hardness_ms));
    }
}
