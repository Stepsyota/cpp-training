#pragma once

#include <condition_variable>
#include <mutex>

// Пассажир: заснуть, пока лифт не приедет. Если лифт уже здесь — не ждать.
void wait_for_elevator(std::mutex& m, std::condition_variable& cv, bool& arrived);

// Кнопка вызова: отметить, что лифт на этаже, и разбудить ждущих.
void call_elevator(std::mutex& m, std::condition_variable& cv, bool& arrived);
