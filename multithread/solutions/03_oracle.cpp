#include "task.hpp"

#include <future>
#include <utility>

std::string ask_oracle(std::function<std::string()> question) {
    return std::async(std::launch::async, std::move(question)).get();
}
