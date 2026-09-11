#pragma once

#include <functional>

// Запускает предполётную проверку в отдельном потоке и возвращается
// только когда проверка полностью закончена.
void run_prelaunch_check(std::function<void()> check);
