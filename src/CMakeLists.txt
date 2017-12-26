set(HOT_SOURCES
    hashedoctree.cpp
    widetree.cpp
    )
if (TBB_FOUND)
  list(APPEND HOT_SOURCES
      hashedoctreeparallel.cpp
      widetreeparallel.cpp
      )
endif ()
add_library(hashedoctree ${HOT_SOURCES})
target_include_directories(hashedoctree
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
    )

target_link_libraries(hashedoctree tbb)
