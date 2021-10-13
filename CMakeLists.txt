cmake_minimum_required(VERSION 2.8)
project(pmem_bench)

# for older version of cmake
macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

use_cxx11()

set(CMAKE_MODULE_PATH, '${CMAKE_CURRENT_LIST_DIR}/cmake')

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPMEM2 libpmem2 REQUIRED IMPORTED_TARGET)
include_directories(${LIBPMEM2_INCLUDE_DIRS})


add_executable(pmem_bench main.cpp spin_barrier.h spin_barrier.cpp)

target_link_libraries(pmem_bench pthread PkgConfig::LIBPMEM2)