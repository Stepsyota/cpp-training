#pragma once

#include "gadget.hpp"

#include <cstddef>
#include <vector>

// Три цеха + две очереди между ними. Очередь пишешь ты.
void run_factory(int n_gadgets, std::size_t queue_capacity, std::vector<Gadget>& packed);
