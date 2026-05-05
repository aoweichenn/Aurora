cmake_minimum_required(VERSION 3.16)
project(Aurora VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" AURORA_SYSTEM_PROCESSOR)
set(AURORA_BUILD_SHARED_LIBS_DEFAULT ON)
if(AURORA_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    set(AURORA_BUILD_SHARED_LIBS_DEFAULT OFF)
endif()

option(AURORA_BUILD_SHARED_LIBS "Build shared libraries" ${AURORA_BUILD_SHARED_LIBS_DEFAULT})
option(AURORA_BUILD_STATIC_LIBS "Build static libraries" ON)
option(AURORA_BUILD_TESTS "Build tests" ON)
option(AURORA_BUILD_BENCHMARKS "Build benchmarks" ON)
option(AURORA_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(AURORA_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$" AND AURORA_BUILD_SHARED_LIBS)
    message(STATUS "Aurora: disabling shared libraries on ARM64 hosts; static tools still build and target x86-64")
    set(AURORA_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
endif()

set(AURORA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(AuroraCommon INTERFACE)
target_include_directories(AuroraCommon INTERFACE ${AURORA_INCLUDE_DIR})
target_compile_options(AuroraCommon INTERFACE
    -Wall -Wextra -Wpedantic
    -Wconversion -Wsign-conversion -Wshadow
    -Wnon-virtual-dtor -Woverloaded-virtual
    -Wold-style-cast -Wcast-align -Wdouble-promotion
    -Wformat=2 -Wmissing-include-dirs
    -Wnull-dereference -Wunused -Wundef
    $<$<CXX_COMPILER_ID:GNU>:-Wlogical-op>
    $<$<CXX_COMPILER_ID:GNU>:-Wstrict-overflow=4>
    $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-branches>
    $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-cond>
    $<$<CXX_COMPILER_ID:GNU>:-Wuseless-cast>
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

# ---- Mini Language Compiler ----
add_subdirectory(tools/minic)

# ---- ELF Object Writer ----
add_subdirectory(tools/aurora-obj)

# ---- Coverage ----
if(AURORA_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(AuroraCommon INTERFACE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(AuroraCommon INTERFACE -fprofile-instr-generate -fcoverage-mapping)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(AuroraCommon INTERFACE -fprofile-arcs -ftest-coverage)
        target_link_options(AuroraCommon INTERFACE -fprofile-arcs -ftest-coverage --coverage)
    endif()
endif()

# ---- Tests ----
if(AURORA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    if(AURORA_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        find_program(LLVM_COV_EXECUTABLE llvm-cov REQUIRED)
        find_program(LLVM_PROFDATA_EXECUTABLE llvm-profdata REQUIRED)
        add_custom_target(minic_coverage
            COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/coverage/llvm_coverage_gate.py
                --llvm-cov ${LLVM_COV_EXECUTABLE}
                --llvm-profdata ${LLVM_PROFDATA_EXECUTABLE}
                --output-dir ${CMAKE_CURRENT_BINARY_DIR}/coverage/minic
                --line-threshold 90
                --branch-threshold 80
                --test $<TARGET_FILE:MiniC_tests>
                --object $<TARGET_FILE:MiniC_tests>
                --source-dir ${CMAKE_CURRENT_SOURCE_DIR}/tools/minic/include/minic
                --source-dir ${CMAKE_CURRENT_SOURCE_DIR}/tools/minic/src
            DEPENDS MiniC_tests
            COMMENT "Running MiniC coverage gate: lines >= 90%, branches >= 80%"
            VERBATIM)
    endif()
endif()
