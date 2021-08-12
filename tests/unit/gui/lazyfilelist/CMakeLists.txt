# define the test executable
add_executable(lazyfilelist_tests ourPosix.cpp strlcpy.c tests.cpp)

# define required search paths
target_include_directories(lazyfilelist_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/gui)

# define custom macros
target_compile_definitions(lazyfilelist_tests PUBLIC LAZYFILELIST_UNITTEST)

# tell build system about the test case
add_catch_test(lazyfilelist_tests)
