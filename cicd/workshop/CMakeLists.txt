cmake_minimum_required(VERSION 3.16)
project(forge_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(forge_cli src/main.cpp)
target_include_directories(forge_cli PRIVATE src)

add_executable(forge_cli_test test/test.cpp)
target_include_directories(forge_cli_test PRIVATE src)

install(TARGETS forge_cli RUNTIME DESTINATION bin)

include(GNUInstallDirs)
include(CPack)

set(CPACK_PACKAGE_NAME "forge-cli")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Forge CLI — учебный артефакт для CI/CD")
set(CPACK_PACKAGE_CONTACT "training@local")
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
