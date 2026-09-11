#include "archive.hpp"
#include "parallel_map.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

int main() {
    using namespace std::chrono_literals;

    {
        const std::vector<int> empty;
        const auto out = parallel_map(empty, seal, 4);
        if (!out.empty()) {
            std::cerr << "FAIL: пустой архив дал " << out.size() << " печатей\n";
            return 1;
        }
    }

    {
        const auto docs = make_archive(200);
        const auto got = parallel_map(docs, seal, 4);
        if (got.size() != docs.size()) {
            std::cerr << "FAIL: размер " << got.size() << ", ждали " << docs.size()
                      << "\n";
            return 1;
        }
        for (std::size_t i = 0; i < docs.size(); ++i) {
            const int want = seal(docs[i]);
            if (got[i] != want) {
                std::cerr << "FAIL: индекс " << i << ": " << got[i] << " != " << want
                          << " (порядок или не тот элемент)\n";
                return 1;
            }
        }
    }

    {
        const std::vector<int> tiny{7, 8};
        const auto got = parallel_map(tiny, seal, 16);
        if (got.size() != 2 || got[0] != seal(7) || got[1] != seal(8)) {
            std::cerr << "FAIL: больше потоков, чем элементов — сломался кусок\n";
            return 1;
        }
    }

    {
        std::mutex mu;
        std::set<std::thread::id> ids;
        const auto main_id = std::this_thread::get_id();
        const std::vector<int> docs(24, 1);
        const auto got = parallel_map(
            docs,
            [&](int x) {
                std::this_thread::sleep_for(8ms);
                {
                    std::lock_guard<std::mutex> lock(mu);
                    ids.insert(std::this_thread::get_id());
                }
                return x + 1;
            },
            4);
        if (got.size() != docs.size()) {
            std::cerr << "FAIL: параллельный прогон вернул " << got.size() << "\n";
            return 1;
        }
        for (int v : got) {
            if (v != 2) {
                std::cerr << "FAIL: ждали 2, получили " << v << "\n";
                return 1;
            }
        }
        if (ids.size() == 1 && *ids.begin() == main_id) {
            std::cerr << "FAIL: всё посчитал вызывающий поток — это обычный map\n";
            return 1;
        }
        if (ids.size() < 2) {
            std::cerr << "FAIL: не увидели несколько рабочих потоков\n";
            return 1;
        }
    }

    std::cout << "OK  08_parallel_map: печати совпали с последовательными, "
                 "работу разрезали по потокам\n";
    return 0;
}
