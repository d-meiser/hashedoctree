add_library(hashedoctree hashedoctree.cpp)
target_include_directories(hashedoctree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(TBB REQUIRED)
target_link_libraries(hashedoctree tbb)
