# Shadows <common/w25x.hpp>; put its include directory before the real one
add_library(w25x-mock INTERFACE)
target_include_directories(
  w25x-mock INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
                      ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )
target_link_libraries(w25x-mock INTERFACE flash-sim-mock)
