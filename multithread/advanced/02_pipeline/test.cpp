#include "factory.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

namespace {

template <typename F>
bool finishes_within(F&& fn, std::chrono::milliseconds limit) {
    std::promise<void> done;
    std::future<void> ready = done.get_future();
    std::thread runner([&] {
        fn();
        done.set_value();
    });
    if (ready.wait_for(limit) != std::future_status::ready) {
        runner.detach();
        return false;
    }
    runner.join();
    return true;
}

bool all_good(const std::vector<Gadget>& packed, int n) {
    if (static_cast<int>(packed.size()) != n) {
        std::cerr << "FAIL: упаковано " << packed.size() << " из " << n << "\n";
        return false;
    }
    std::vector<bool> seen(static_cast<std::size_t>(n), false);
    for (const Gadget& g : packed) {
        if (g.serial < 0 || g.serial >= n) {
            std::cerr << "FAIL: странный серийник " << g.serial << "\n";
            return false;
        }
        if (seen[static_cast<std::size_t>(g.serial)]) {
            std::cerr << "FAIL: деталь " << g.serial << " пришла дважды\n";
            return false;
        }
        seen[static_cast<std::size_t>(g.serial)] = true;
        if (!g.stamped || !g.painted) {
            std::cerr << "FAIL: деталь " << g.serial
                      << " не прошла оба цеха (stamped=" << g.stamped
                      << " painted=" << g.painted << ")\n";
            return false;
        }
    }
    return true;
}

}  // namespace

int main() {
    using namespace std::chrono_literals;

    {
        std::vector<Gadget> packed;
        if (!finishes_within([&] { run_factory(50, 8, packed); }, 3s)) {
            std::cerr << "FAIL: конвейер завис на 50 деталях (close/notify/предикат?)\n";
            return 1;
        }
        if (!all_good(packed, 50)) {
            return 1;
        }
    }

    {
        std::vector<Gadget> packed;
        if (!finishes_within([&] { run_factory(30, 1, packed); }, 3s)) {
            std::cerr << "FAIL: узкая очередь (ёмкость 1) зависла\n";
            return 1;
        }
        if (!all_good(packed, 30)) {
            return 1;
        }
    }

    {
        std::vector<Gadget> packed;
        if (!finishes_within([&] { run_factory(0, 4, packed); }, 2s)) {
            std::cerr << "FAIL: пустая партия зависла — close должен будить pop\n";
            return 1;
        }
        if (!packed.empty()) {
            std::cerr << "FAIL: пустая партия дала " << packed.size() << " деталей\n";
            return 1;
        }
    }

    std::cout << "OK  07_pipeline: три цеха прогнали партию через твою очередь\n";
    return 0;
}
