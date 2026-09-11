#include "task.hpp"

void consider_score(std::atomic<int>& highscore, int score) {
    // TODO: атомарно обнови рекорд, если score его бьёт.
    // Наивный вариант:
    //     if (score > highscore.load()) highscore.store(score);
    // ломается: между load и store другой поток может записать ещё большее
    // значение, и ты его затрёшь меньшим.
    (void)highscore;
    (void)score;

    int current = highscore.load();
    while (score > current && !highscore.compare_exchange_weak(current, score)) {
        
    }
}
