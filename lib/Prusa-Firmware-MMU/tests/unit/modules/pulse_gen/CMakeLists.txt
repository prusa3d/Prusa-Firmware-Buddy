# define the test executable
add_executable(
  pulse_gen_tests
  ${CMAKE_SOURCE_DIR}/src/modules/pulse_gen.cpp ${CMAKE_SOURCE_DIR}/src/modules/speed_table.cpp
  ${MODULES_STUBS_DIR}/stub_gpio.cpp ${MODULES_STUBS_DIR}/stub_shr16.cpp test_pulse_gen.cpp
  )

# define required search paths
target_include_directories(
  pulse_gen_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/hal
  )

# tell build system about the test case
add_catch_test(pulse_gen_tests)
