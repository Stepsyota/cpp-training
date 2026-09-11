#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

// Ограниченная очередь: конвейер между двумя цехами.
// push блокируется, если мест нет; pop — если очередь пуста.
// close() говорит потребителям: новых деталей не будет.
//
// Шаблон — весь код в этом заголовке.
template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : cap_(capacity == 0 ? 1 : capacity) {}

    void push(T item) {
        // TODO: жди, пока появится место (или очередь закрыли).
        // Если закрыта — новую деталь не клади.
        // После успешного push разбуди тех, кто ждёт pop.

        {
            std::unique_lock<std::mutex> lock(m);
            not_full.wait(lock, [this] {return q.size() < cap_ || closed;});
    
            if (closed) {
                return;
            }
    
            q.push(std::move(item));
        }
        not_empty.notify_one();
    }

    // true — забрали деталь. false — очередь закрыта и пуста, конвейер кончился.
    bool pop(T& out) {
        // TODO: жди, пока появится деталь или close().

        {
            std::unique_lock<std::mutex> lock(m);
            not_empty.wait(lock, [this] {return !q.empty() || closed;});
    
            if (closed && q.empty()) {
                return false;
            }
    
            out = std::move(q.front());
            q.pop();
            
            not_full.notify_one();
        }
        return true;
    }

    void close() {
        // TODO: поставь флаг и разбуди всех, кто спит на push/pop.

        {
            std::lock_guard<std::mutex> lock(m);
            closed = true;
        }

        not_empty.notify_all();
        not_full.notify_all();
    }

private:
    std::size_t cap_;

    std::queue<T> q;
    std::mutex m;
    std::condition_variable not_full;
    std::condition_variable not_empty;

    bool closed = false;
    // очередь, мьютекс, две cv (не полна / не пуста), флаг closed
};
