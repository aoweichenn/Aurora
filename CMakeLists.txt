cmake_minimum_required(VERSION 3.16)
project(Aurora VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AURORA_BUILD_SHARED_LIBS "Build shared libraries" ON)
option(AURORA_BUILD_STATIC_LIBS "Build static libraries" ON)
option(AURORA_BUILD_TESTS "Build tests" OFF)

set(AURORA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(AuroraCommon INTERFACE)
target_include_directories(AuroraCommon INTERFACE ${AURORA_INCLUDE_DIR})
target_compile_options(AuroraCommon INTERFACE
    -Wall -Wextra -Wpedantic -Wno-unused-parameter
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

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
