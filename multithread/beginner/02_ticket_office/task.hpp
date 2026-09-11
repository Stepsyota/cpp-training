#pragma once

#include <mutex>

// Одна продажа билета. Несколько касс зовут эту функцию одновременно.
void sell_ticket(int& tickets_sold, std::mutex& cash_register);
