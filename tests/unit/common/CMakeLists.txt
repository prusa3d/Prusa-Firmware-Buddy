
# define the test executable
add_executable(gcode_tests gcode_test.cpp)

# define required search paths
target_include_directories(gcode_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)

# define custom macros
target_compile_definitions(gcode_tests PUBLIC STR_UTILS_UNITTEST)

# tell build system about the test case
add_catch_test(gcode_tests)

########################################################

# define the test executable
add_executable(str_utils_tests str_utils_test.cpp)

# define required search paths
target_include_directories(str_utils_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)

# define custom macros
target_compile_definitions(str_utils_tests PUBLIC STR_UTILS_UNITTEST)

# tell build system about the test case
add_catch_test(str_utils_tests)
