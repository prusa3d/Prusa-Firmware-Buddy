# define the test executable
add_executable(
  speed_table_tests ${CMAKE_SOURCE_DIR}/src/modules/speed_table.cpp test_speed_table.cpp
  )

# define required search paths
target_include_directories(speed_table_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/modules)

# tell build system about the test case
add_catch_test(speed_table_tests)
