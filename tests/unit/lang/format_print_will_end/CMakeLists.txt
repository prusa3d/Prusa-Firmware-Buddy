# define the test executable
add_executable(
  format_print_will_end_tests ../../../../src/lang/format_print_will_end.cpp
                              ../../../../src/lang/translator.cpp tests.cpp
  )

# define required search paths
target_include_directories(
  format_print_will_end_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/lang ${CMAKE_SOURCE_DIR}/include
  )

# define custom macros

# target_compile_definitions(format_print_will_end_tests PUBLIC)

# tell build system about the test case
add_catch_test(format_print_will_end_tests)
