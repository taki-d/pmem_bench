cmake_minimum_required(VERSION 3.21)
project(pmem_bench)

set(CMAKE_CXX_STANDARD 14)

add_executable(pmem_bench main.cpp spin_barrier.h spin_barrier.cpp)
target_link_libraries(pmem_bench pmem2 pthread)