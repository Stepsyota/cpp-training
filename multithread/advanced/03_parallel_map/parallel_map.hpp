#pragma once

#include <cstddef>
#include <algorithm>
#include <type_traits>
#include <vector>
#include <thread>

// Применить fn к каждому элементу. Порядок в ответе как во входе.
// n_threads рабочих режут диапазон и пишут каждый в свой кусок out[i].
// Разные индексы — разные ячейки, мьютекс на весь вектор не нужен.
template <typename T, typename F>
std::vector<std::invoke_result_t<F, const T&>> parallel_map(
    const std::vector<T>& input,
    F fn,
    std::size_t n_threads) {
    // TODO: пустой вход → пустой выход.
    // n_threads == 0 считай за 1. Больше элементов, чем потоков — ок,
    // больше потоков, чем элементов — не создавай пустых рабочих.
    // Запусти потоки, каждый считает свой полуинтервал [begin, end),
    // потом join всех. Не вызывай fn только из текущего потока.

    if (n_threads == 0) n_threads = 1;
    n_threads = std::min(n_threads, input.size());

    std::vector<std::invoke_result_t<F, const T&>> out(input.size());
    std::vector<std::thread> threads (n_threads);


    // Кусками
    for (std::size_t i = 0; i < n_threads; ++i) {
        threads[i] = std::move(
            std::thread(
                [&input, &fn, &out, n_threads, i] {
                    std::size_t lower = i * input.size() / n_threads;
                    std::size_t higher = (i + 1) * input.size() / n_threads;

                    for (std::size_t j = lower; j < higher; ++j) {
                        out[j] = fn(input[j]);
                    }
                }
            )
        );
        
    }

    // Шагом
    // for (std::size_t i = 0; i < threads.size(); ++i) {
    //     threads[i] = std::move(
    //         std::thread(
    //             [&input, &fn, &out, n_threads, i] {
    //                 for (std::size_t j = i; j < out.size(); j += n_threads) {
    //                     out[j] = fn(input[j]);
    //                 }
    //             }
    //         )
    //     );
    // }
    

    for (auto& thread : threads) {
        thread.join();
    }

    return out;
}