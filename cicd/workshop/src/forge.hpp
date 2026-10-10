#pragma once

#include <iostream>
#include <string_view>

namespace forge {

inline constexpr std::string_view kVersion = "0.1.0";

inline int run(std::string_view command) {
    if (command == "--version") {
        std::cout << "forge-cli " << kVersion << '\n';
        return 0;
    }
    if (command == "ping") {
        std::cout << "pong\n";
        return 0;
    }
    std::cerr << "unknown command: " << command << '\n';
    return 1;
}

}  // namespace forge
