add_executable(hashedoctree_test hashedoctree_test.cpp)
target_link_libraries(hashedoctree_test hashedoctree gtest_main ${COV_LIBRARIES})
add_test(hashedoctree_test hashedoctree_test)

