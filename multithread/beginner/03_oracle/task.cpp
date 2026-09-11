#include "task.hpp"
#include <future>

std::string ask_oracle(std::function<std::string()> question) {
    // TODO: выполни question в другом потоке и верни то, что он вернул.
    // Просто вызвать question() в этом потоке — недостаточно.
    std::string answer = std::async(std::launch::async, question).get();
    return answer;
}
