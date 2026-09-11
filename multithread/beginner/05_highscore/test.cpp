#include "task.hpp"

#include <iostream>
#include <thread>
#include <vector>

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::cerr << "FAIL: " #cond << "  (" << __FILE__ << ":"          \
                      << __LINE__ << ")\n";                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main() {
    {
        std::atomic<int> highscore{100};
        consider_score(highscore, 50);
        CHECK(highscore.load() == 100);
        consider_score(highscore, 150);
        CHECK(highscore.load() == 150);
    }

    constexpr int kThreads = 32;
    constexpr int kPerThread = 1000;
    constexpr int kRounds = 8;

    for (int round = 0; round < kRounds; ++round) {
        std::atomic<int> highscore{0};
        std::vector<std::thread> players;
        players.reserve(kThreads);

        for (int i = 0; i < kThreads; ++i) {
            players.emplace_back([&highscore, i] {
                for (int k = 0; k < kPerThread; ++k) {
                    consider_score(highscore, i * kPerThread + k);
                }
            });
        }
        for (auto& t : players) {
            t.join();
        }

        const int expected = (kThreads - 1) * kPerThread + (kPerThread - 1);
        if (highscore.load() != expected) {
            std::cerr << "FAIL: рекорд " << highscore.load() << ", ждали "
                      << expected << " (раунд " << round << ")\n";
            return 1;
        }
    }

    std::cout << "OK  05_highscore: меньший счёт не затирает рекорд, "
                 "гонки не съедают максимум\n";
    return 0;
}
