add_executable(
  path_tests ${CMAKE_CURRENT_SOURCE_DIR}/path_test.cpp ${CMAKE_SOURCE_DIR}/lib/libsysbase/path.c
  )
target_include_directories(path_tests PUBLIC ${CMAKE_SOURCE_DIR}/lib/libsysbase/)

add_catch_test(path_tests)
