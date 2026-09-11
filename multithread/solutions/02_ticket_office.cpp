#include "task.hpp"

void sell_ticket(int& tickets_sold, std::mutex& cash_register) {
    std::lock_guard<std::mutex> lock(cash_register);
    ++tickets_sold;
}
