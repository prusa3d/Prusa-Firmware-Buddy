# define the test executable
add_executable(
  user_input_tests
  ${CMAKE_SOURCE_DIR}/src/modules/buttons.cpp ${CMAKE_SOURCE_DIR}/src/modules/debouncer.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/user_input.cpp ${MODULES_STUBS_DIR}/stub_adc.cpp
  ${MODULES_STUBS_DIR}/stub_timebase.cpp test_user_input.cpp
  )

# define required search paths
target_include_directories(
  user_input_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/hal
  )

# tell build system about the test case
add_catch_test(user_input_tests)
