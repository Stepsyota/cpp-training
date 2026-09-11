#include "task.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <thread>

int main() {
    using namespace std::chrono_literals;

    {
        std::mutex m;
        std::condition_variable cv;
        bool arrived = false;
        std::promise<void> done;
        std::future<void> woke = done.get_future();

        std::thread passenger([&] {
            wait_for_elevator(m, cv, arrived);
            done.set_value();
        });

        std::this_thread::sleep_for(60ms);
        if (woke.wait_for(0ms) == std::future_status::ready) {
            passenger.join();
            std::cerr << "FAIL: wait_for_elevator вернулся до call_elevator\n";
            return 1;
        }

        call_elevator(m, cv, arrived);

        if (woke.wait_for(2s) != std::future_status::ready) {
            std::cerr << "FAIL: wait_for_elevator не проснулся после "
                         "call_elevator (таймаут 2с)\n";
            passenger.detach();
            return 1;
        }
        passenger.join();
        if (!arrived) {
            std::cerr << "FAIL: arrived должен стать true после вызова\n";
            return 1;
        }
    }

    {
        std::mutex m;
        std::condition_variable cv;
        bool arrived = false;
        std::promise<void> done;
        std::future<void> woke = done.get_future();

        call_elevator(m, cv, arrived);

        std::thread late_passenger([&] {
            wait_for_elevator(m, cv, arrived);
            done.set_value();
        });

        if (woke.wait_for(2s) != std::future_status::ready) {
            std::cerr << "FAIL: сигнал потерян — wait без проверки arrived "
                         "зависает, если call_elevator был раньше\n";
            late_passenger.detach();
            return 1;
        }
        late_passenger.join();
    }

    std::cout << "OK  04_elevator: ждущие просыпаются, поздний пассажир "
                 "не теряет уже приехавший лифт\n";
    return 0;
}
