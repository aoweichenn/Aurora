cmake_minimum_required(VERSION 3.16)
project(Aurora VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AURORA_BUILD_SHARED_LIBS "Build shared libraries" ON)
option(AURORA_BUILD_STATIC_LIBS "Build static libraries" ON)
option(AURORA_BUILD_TESTS "Build tests" ON)
option(AURORA_BUILD_BENCHMARKS "Build benchmarks" ON)

set(AURORA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(AuroraCommon INTERFACE)
target_include_directories(AuroraCommon INTERFACE ${AURORA_INCLUDE_DIR})
target_compile_options(AuroraCommon INTERFACE
    -Wall -Wextra -Wpedantic
    -Wconversion -Wsign-conversion -Wshadow
    -Wnon-virtual-dtor -Woverloaded-virtual
    -Wold-style-cast -Wcast-align -Wdouble-promotion
    -Wformat=2 -Wlogical-op -Wmissing-include-dirs
    -Wnull-dereference -Wstrict-overflow=4
    -Wduplicated-branches -Wduplicated-cond
    -Wuseless-cast -Wunused -Wundef
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# ---- Google Test ----
if(AURORA_BUILD_TESTS OR AURORA_BUILD_BENCHMARKS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# ---- Google Benchmark ----
if(AURORA_BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
endif()

# ---- ADT ----
add_subdirectory(lib/ADT)

# ---- Air ----
add_subdirectory(lib/Air)

# ---- Target ----
add_subdirectory(lib/Target)

# ---- CodeGen ----
add_subdirectory(lib/CodeGen)

# ---- MC ----
add_subdirectory(lib/MC)

# ---- Driver ----
add_subdirectory(tools/aurorac)

# ---- Tests ----
if(AURORA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
