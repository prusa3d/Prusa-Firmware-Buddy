add_subdirectory(str_utils)

# define required search paths
target_include_directories(support_utils_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/common)

# define custom macros
target_compile_definitions(support_utils_tests PUBLIC SUPPORT_UTILS_UNITTEST)

# tell build system about the test case
add_catch_test(support_utils_tests)
