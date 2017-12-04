cmake_minimum_required(VERSION 3.0.0)
project(HashedOctree)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
enable_testing()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

add_subdirectory(googletest-release-1.8.0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(hashedoctree hashedoctree.cpp)

add_executable(hashedoctree_test hashedoctree_test.cpp)
target_link_libraries(hashedoctree_test hashedoctree gtest_main)
add_test(hashedoctree_test hashedoctree_test)
