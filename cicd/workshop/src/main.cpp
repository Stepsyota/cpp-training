#include "forge.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: forge-cli <command>\n";
        return 1;
    }
    return forge::run(argv[1]);
}
