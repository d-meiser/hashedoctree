cmake_minimum_required(VERSION 3.0.0)
project(HashedOctree)
enable_testing()

option(HOT_ENABLE_COVERAGE "Instrument tests for coverage" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

if (HOT_ENABLE_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ftest-coverage -fprofile-arcs")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage -fprofile-arcs")
  set(COV_LIBRARIES gcov)
endif ()

add_subdirectory(googletest-release-1.8.0)
find_package(TBB)
add_subdirectory(src)
add_subdirectory(tests)

if (TBB_FOUND)
  set(HOT_HAVE_TBB ON)
endif ()
configure_file(hot_config.h.in hot_config.h)
