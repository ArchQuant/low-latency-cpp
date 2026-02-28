cmake_minimum_required(VERSION 3.20)
project(module2_demos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Build type defaults to Release for accurate benchmarks ──
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# Release flags for accurate benchmarking
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# ── Exercise targets ──
add_executable(ex1_reallocation ex1_reallocation.cpp)
add_executable(ex2_vector_vs_list ex2_vector_vs_list.cpp)
add_executable(ex3_soa_vs_aos ex3_soa_vs_aos.cpp)
add_executable(ex4_order_buffer ex4_order_buffer.cpp)
