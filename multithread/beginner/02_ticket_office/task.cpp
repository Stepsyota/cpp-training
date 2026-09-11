#include "task.hpp"
#include <mutex>

void sell_ticket(int& tickets_sold, std::mutex& cash_register) {
    // TODO: увеличь tickets_sold на 1. Несколько потоков делают это сразу —
    // без блокировки часть продаж потеряется.
    std::lock_guard<std::mutex> lock(cash_register);
    ++tickets_sold;
}
