cmake_minimum_required(VERSION 3.16)
project(market-maker-bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/lob-matching-engine)

add_library(mm_core STATIC
    src/noise_trader.cpp
    src/fair_value_estimator.cpp
    src/market_maker.cpp
    src/static_spread_strategy.cpp
    src/risk_manager.cpp
    src/pnl_tracker.cpp
    src/backtest_engine.cpp
)
target_include_directories(mm_core PUBLIC include)
target_link_libraries(mm_core PUBLIC lob_core)
target_compile_options(mm_core PRIVATE -Wall -Wextra -O2)

add_executable(mm_main src/main.cpp)
target_link_libraries(mm_main PRIVATE mm_core)

# Catch2
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

enable_testing()
add_executable(mm_tests
    tests/test_fair_value.cpp
    tests/test_quoting.cpp
    tests/test_risk_limits.cpp
    tests/test_pnl_tracker.cpp
)
target_link_libraries(mm_tests PRIVATE mm_core Catch2::Catch2WithMain)
target_compile_options(mm_tests PRIVATE -Wall -Wextra -O2)
catch_discover_tests(mm_tests)
