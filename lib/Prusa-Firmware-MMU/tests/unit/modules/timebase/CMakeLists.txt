# define the test executable
add_executable(timebase_tests ${MODULES_STUBS_DIR}/stub_timebase.cpp test_timebase.cpp)

# define required search paths
target_include_directories(timebase_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/modules)

# tell build system about the test case
add_catch_test(timebase_tests)
