#pragma once

#include <vector>

// Архив свитков. Не меняй.
inline std::vector<int> make_archive(int n) {
    std::vector<int> docs(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        docs[static_cast<std::size_t>(i)] = i + 1;
    }
    return docs;
}

inline int seal(int doc_id) {
    int x = doc_id;
    for (int i = 0; i < 200; ++i) {
        x = x * 1103515245 + 12345;
    }
    return x ^ doc_id;
}
