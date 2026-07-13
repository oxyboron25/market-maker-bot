cmake_minimum_required(VERSION 3.16)
project(lob-matching-engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core library
add_library(lob_core STATIC
    src/order_book.cpp
    src/matching_engine.cpp
)
target_include_directories(lob_core PUBLIC include)
target_compile_options(lob_core PRIVATE -Wall -Wextra -Wpedantic -O2)

# Main executable
add_executable(lob_main src/main.cpp)
target_link_libraries(lob_main PRIVATE lob_core)

# Catch2 via FetchContent
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# Tests
enable_testing()
add_executable(lob_tests
    tests/test_order_book.cpp
    tests/test_matching.cpp
    tests/test_edge_cases.cpp
)
target_link_libraries(lob_tests PRIVATE lob_core Catch2::Catch2WithMain)
target_compile_options(lob_tests PRIVATE -Wall -Wextra -O2)
catch_discover_tests(lob_tests)

# Benchmark
add_executable(lob_bench bench/benchmark_throughput.cpp)
target_link_libraries(lob_bench PRIVATE lob_core)
target_compile_options(lob_bench PRIVATE -Wall -O2)
