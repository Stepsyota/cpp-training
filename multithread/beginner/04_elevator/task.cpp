#include "task.hpp"
#include <mutex>

void wait_for_elevator(std::mutex& m, std::condition_variable& cv, bool& arrived) {
    // TODO: подожди, пока arrived не станет true.
    // Важно: если лифт вызвали ДО того, как ты начал ждать, всё равно
    // нужно сразу пройти — иначе сигнал «потеряется».
    (void)m;
    (void)cv;
    (void)arrived;
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&arrived] {return arrived;});
}

void call_elevator(std::mutex& m, std::condition_variable& cv, bool& arrived) {
    // TODO: поставь arrived = true (под замком) и разбуди ждущих.
    (void)m;
    (void)cv;
    (void)arrived;

    {
        std::lock_guard<std::mutex> lock(m);
        arrived = true;
    }
    
    cv.notify_all();
}
