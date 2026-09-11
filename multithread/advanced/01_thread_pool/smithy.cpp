#include "smithy.hpp"

void run_shift(ThreadPool& pool,
               const std::vector<Order>& orders,
               std::atomic<int>& completed) {
    for (const Order& order : orders) {
        pool.submit([order, &completed] {
            fulfill(order);
            completed.fetch_add(1);
        });
    }
}
