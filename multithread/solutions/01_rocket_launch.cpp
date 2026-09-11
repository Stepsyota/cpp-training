#include "task.hpp"

#include <thread>
#include <utility>

void run_prelaunch_check(std::function<void()> check) {
    std::thread officer(std::move(check));
    officer.join();
}
