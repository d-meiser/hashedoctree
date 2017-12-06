cmake_minimum_required(VERSION 3.0.0)
project(HashedOctree)
enable_testing()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

add_subdirectory(googletest-release-1.8.0)

add_subdirectory(src)
add_subdirectory(test)
