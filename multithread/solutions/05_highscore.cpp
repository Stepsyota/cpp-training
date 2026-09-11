#include "task.hpp"

void consider_score(std::atomic<int>& highscore, int score) {
    int current = highscore.load();
    while (score > current &&
           !highscore.compare_exchange_weak(current, score)) {
    }
}
