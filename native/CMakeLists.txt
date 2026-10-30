cmake_minimum_required(VERSION 3.20)
project(hijaz_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto)

add_library(hijaz_core
    src/hash.cpp
    src/transaction.cpp
    src/block.cpp
    src/ledger.cpp
    src/block_validation.cpp
    src/p2p_frame.cpp
)
target_include_directories(hijaz_core PUBLIC include)
target_link_libraries(hijaz_core PUBLIC OpenSSL::Crypto)
target_compile_options(hijaz_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>
)

add_executable(hijaz_core_tests tests/core_tests.cpp)
target_link_libraries(hijaz_core_tests PRIVATE hijaz_core)

enable_testing()
add_test(NAME hijaz_core_tests COMMAND hijaz_core_tests)
