#pragma once

#include <atomic>

// Если score больше текущего рекорда — записать его.
// Несколько игроков вызывают функцию одновременно.
void consider_score(std::atomic<int>& highscore, int score);
