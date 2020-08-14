
# define the test executable
add_executable(str_utils_tests
    str_utils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
)

# define required search paths
target_include_directories(str_utils_tests PUBLIC .
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src
)

# define custom macros
target_compile_definitions(str_utils_tests PUBLIC STR_UTILS_UNITTEST)

# tell build system about the test case
add_catch_test(str_utils_tests)
