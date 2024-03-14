# define the test executable
add_executable(neopixel_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests_neopixel.cpp)

# define required search paths
target_include_directories(neopixel_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/hw)

# tell build system about the test case
add_catch_test(neopixel_tests)
