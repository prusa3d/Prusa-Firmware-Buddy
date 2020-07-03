
# define the test executable
add_executable(translator_tests tests.cpp)

# define required search paths
target_include_directories(translator_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/lang)

# define custom macros
# target_compile_definitions(translator_tests PUBLIC LAZYFILELIST_UNITTEST)

# tell build system about the test case
add_catch_test(translator_tests)
