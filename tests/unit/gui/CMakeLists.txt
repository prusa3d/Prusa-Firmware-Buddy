
# define the test executable
add_executable(rectangle_tests
    rectangle_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/test_main.cpp
)

# define required search paths
target_include_directories(rectangle_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/guiapi/include
)

# tell build system about the test case
add_catch_test(rectangle_tests)

add_subdirectory(lazyfilelist)
