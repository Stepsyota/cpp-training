#include "task.hpp"
#include <thread>

void run_prelaunch_check(std::function<void()> check) {
    // TODO: запусти check в отдельном std::thread и дождись окончания (join).
    // Синхронный вызов check() здесь — неправильно: проверка должна идти
    // в другом потоке.
    std::thread t(check);
    t.join();
}
