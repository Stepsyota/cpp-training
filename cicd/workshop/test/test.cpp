#include "forge.hpp"

#include <iostream>
#include <string>

static int fail(const char* msg) {
    std::cerr << "FAIL: " << msg << '\n';
    return 1;
}

int main() {
    if (forge::run("ping") != 0) {
        return fail("ping");
    }
    if (forge::run("--version") != 0) {
        return fail("--version exit code");
    }
    if (std::string(forge::kVersion) != "0.1.0") {
        return fail("kVersion");
    }
    std::cout << "OK\n";
    return 0;
}
