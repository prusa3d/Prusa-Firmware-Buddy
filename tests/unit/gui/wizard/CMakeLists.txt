
# define the test executable
add_executable(firslay_tests firstlay_test.cpp)

# define required search paths
target_include_directories(firslay_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/gui/wizard)

# define custom macros
target_compile_definitions(firslay_tests  PUBLIC FIRSTLAY_UNITTEST)

# tell build system about the test case
add_catch_test(firslay_tests)
