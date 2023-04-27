# define the test executable
add_executable(
  buttons_tests
  ${CMAKE_SOURCE_DIR}/src/modules/buttons.cpp ${CMAKE_SOURCE_DIR}/src/modules/debouncer.cpp
  ${MODULES_STUBS_DIR}/stub_adc.cpp ${MODULES_STUBS_DIR}/stub_timebase.cpp test_buttons.cpp
  )

# define required search paths
target_include_directories(
  buttons_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/hal
  )

# tell build system about the test case
add_catch_test(buttons_tests)
