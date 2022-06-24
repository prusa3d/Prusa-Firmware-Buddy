# define the test executable
add_executable(wizard_tests tests_wizard_types.cpp)

# define required search paths
target_include_directories(
  wizard_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/gui/wizard ${CMAKE_SOURCE_DIR}/src/gui
  )

# define custom macros
target_compile_definitions(wizard_tests PUBLIC WIZARD_UNITTEST)

# tell build system about the test case
add_catch_test(wizard_tests)
