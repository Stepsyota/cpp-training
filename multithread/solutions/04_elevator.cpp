#include "task.hpp"

void wait_for_elevator(std::mutex& m, std::condition_variable& cv, bool& arrived) {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&] { return arrived; });
}

void call_elevator(std::mutex& m, std::condition_variable& cv, bool& arrived) {
    {
        std::lock_guard<std::mutex> lock(m);
        arrived = true;
    }
    cv.notify_all();
}
