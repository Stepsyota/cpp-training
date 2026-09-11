cmake_minimum_required(VERSION 3.16)
project(multithread_training LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

function(add_exercise name)
    add_executable(${name}
        beginner/${name}/task.cpp
        beginner/${name}/test.cpp
    )
    target_include_directories(${name} PRIVATE beginner/${name})
    target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()

add_exercise(01_rocket_launch)
add_exercise(02_ticket_office)
add_exercise(03_oracle)
add_exercise(04_elevator)
add_exercise(05_highscore)

add_executable(06_thread_pool
    advanced/01_thread_pool/thread_pool.cpp
    advanced/01_thread_pool/smithy.cpp
    advanced/01_thread_pool/test.cpp
)
target_include_directories(06_thread_pool PRIVATE advanced/01_thread_pool)
target_link_libraries(06_thread_pool PRIVATE Threads::Threads)

add_executable(07_pipeline
    advanced/02_pipeline/factory.cpp
    advanced/02_pipeline/test.cpp
)
target_include_directories(07_pipeline PRIVATE advanced/02_pipeline)
target_link_libraries(07_pipeline PRIVATE Threads::Threads)

add_executable(08_parallel_map
    advanced/03_parallel_map/test.cpp
)
target_include_directories(08_parallel_map PRIVATE advanced/03_parallel_map)
target_link_libraries(08_parallel_map PRIVATE Threads::Threads)
