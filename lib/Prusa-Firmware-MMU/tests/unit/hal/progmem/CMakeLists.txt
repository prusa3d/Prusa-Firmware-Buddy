# define the test executable
add_executable(progmem_tests test_progmem.cpp)

# define required search paths
target_include_directories(progmem_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/hal)

# tell build system about the test case
add_catch_test(progmem_tests)
