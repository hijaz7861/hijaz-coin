cmake_minimum_required(VERSION 3.20)
project(hijaz_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto)

# OpenSSL's standardized ML-DSA EVP implementation was added in OpenSSL 3.5.
if(OPENSSL_VERSION VERSION_GREATER_EQUAL "3.5.0")
    set(HJZ_OPENSSL_ML_DSA_AVAILABLE ON)
else()
    set(HJZ_OPENSSL_ML_DSA_AVAILABLE OFF)
    message(STATUS "OpenSSL < 3.5: provider-backed ML-DSA support will be disabled")
endif()

add_library(hijaz_core
    src/hash.cpp
    src/transaction.cpp
    src/block.cpp
    src/block_wire.cpp
    src/ledger.cpp
    src/block_validation.cpp
    src/block_execution.cpp
    src/storage.cpp
    src/p2p_frame.cpp
)
if(HJZ_OPENSSL_ML_DSA_AVAILABLE)
    target_sources(hijaz_core PRIVATE src/openssl_ml_dsa.cpp)
    target_compile_definitions(hijaz_core PUBLIC HJZ_HAS_OPENSSL_ML_DSA=1)
endif()

target_include_directories(hijaz_core PUBLIC include)
target_link_libraries(hijaz_core PUBLIC OpenSSL::Crypto)
target_compile_options(hijaz_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>
)

add_executable(hijaz_core_tests tests/core_tests.cpp)
target_link_libraries(hijaz_core_tests PRIVATE hijaz_core)
# The test suite uses assert(); keep assertions active even in Release builds.
target_compile_options(hijaz_core_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
)

enable_testing()
add_test(NAME hijaz_core_tests COMMAND hijaz_core_tests)
