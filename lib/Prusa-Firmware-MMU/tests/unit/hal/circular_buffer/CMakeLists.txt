# define the test executable
add_executable(circular_buffer_tests test_circular_buffer.cpp)

# define required search paths
target_include_directories(circular_buffer_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/hal)

# tell build system about the test case
add_catch_test(circular_buffer_tests)
