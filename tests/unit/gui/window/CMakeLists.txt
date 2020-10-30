# define the test executable
add_executable(
  window_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests_window.cpp ${CMAKE_SOURCE_DIR}/src/guiapi/src/window.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/window_frame.cpp ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
  )

# define required search paths
target_include_directories(window_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/guiapi/include)
target_include_directories(window_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)

# tell build system about the test case
add_catch_test(window_tests)
