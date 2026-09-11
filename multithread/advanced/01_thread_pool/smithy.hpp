#pragma once

#include "order.hpp"
#include "thread_pool.hpp"

#include <atomic>
#include <vector>

// Кузница умеет только сдавать заказы в пул. Очередь и воркеры — твои.
void run_shift(ThreadPool& pool,
               const std::vector<Order>& orders,
               std::atomic<int>& completed);
