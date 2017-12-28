add_library(test_utilities test_utilities.cpp)
target_include_directories(test_utilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_utilities hashedoctree)

foreach (t hashedoctree widetree tree)
  add_executable(${t}_test ${t}_test.cpp)
  target_link_libraries(${t}_test hashedoctree test_utilities gtest_main ${COV_LIBRARIES})
  add_test(${t}_test ${t}_test)
endforeach ()

foreach (t vertex_dedup_test counting_sort_test)
  add_executable(${t} ${t}.cpp)
  target_link_libraries(${t} hashedoctree test_utilities)
  if (TBB_FOUND)
    target_link_libraries(${t} tbb)
  endif()
endforeach ()

