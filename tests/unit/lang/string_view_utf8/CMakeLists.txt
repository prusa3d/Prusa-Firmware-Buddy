
# define the test executable
add_executable(string_view_utf8_tests tests.cpp)

# define required search paths
target_include_directories(string_view_utf8_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/lang)

# define custom macros
# target_compile_definitions(string_view_utf8_tests PUBLIC LAZYFILELIST_UNITTEST)

# tell build system about the test case
add_catch_test(string_view_utf8_tests)
