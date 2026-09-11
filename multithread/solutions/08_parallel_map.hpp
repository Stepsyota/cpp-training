#pragma once

#include <algorithm>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <vector>

template <typename T, typename F>
std::vector<std::invoke_result_t<F, const T&>> parallel_map(
    const std::vector<T>& input,
    F fn,
    std::size_t n_threads) {
    using Out = std::invoke_result_t<F, const T&>;
    const std::size_t n = input.size();
    if (n == 0) {
        return {};
    }
    if (n_threads == 0) {
        n_threads = 1;
    }
    n_threads = std::min(n_threads, n);

    std::vector<Out> out(n);
    std::vector<std::thread> workers;
    workers.reserve(n_threads);

    for (std::size_t t = 0; t < n_threads; ++t) {
        const std::size_t begin = t * n / n_threads;
        const std::size_t end = (t + 1) * n / n_threads;
        workers.emplace_back([&, begin, end] {
            for (std::size_t i = begin; i < end; ++i) {
                out[i] = fn(input[i]);
            }
        });
    }
    for (std::thread& w : workers) {
        w.join();
    }
    return out;
}
