# common include and source directories
add_compile_definitions(UNITTEST_MOTION)
set(include_common ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/hal)
set(source_common
    ${CMAKE_SOURCE_DIR}/src/modules/motion.cpp
    ${CMAKE_SOURCE_DIR}/src/modules/speed_table.cpp
    ${CMAKE_SOURCE_DIR}/src/modules/pulse_gen.cpp
    ${MODULES_STUBS_DIR}/stub_gpio.cpp
    ${MODULES_STUBS_DIR}/stub_panic.cpp
    ${CMAKE_SOURCE_DIR}/src/hal/tmc2130.cpp
    ${MODULES_STUBS_DIR}/stub_shr16.cpp
    ${MODULES_STUBS_DIR}/stub_spi.cpp
    )

# general motion tests
add_executable(motion_tests ${source_common} test_motion.cpp)
target_include_directories(motion_tests PUBLIC ${include_common})
add_catch_test(motion_tests)

# ramp generation tests
add_executable(rampgen ${source_common} rampgen.cpp)
target_include_directories(rampgen PUBLIC ${include_common})
add_test(NAME motion::rampgen COMMAND rampgen ramp_data.txt)
add_dependencies(tests rampgen)

execute_process(
  COMMAND "${Python3_EXECUTABLE}" -c "import pandas"
  RESULT_VARIABLE PANDAS_INSTALLED
  OUTPUT_QUIET ERROR_QUIET
  )
if(${PANDAS_INSTALLED} EQUAL 0)
  # only run test_motion_ramp if pandas is installed
  add_test(NAME motion::test_motion_ramp
           COMMAND "${Python3_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/test_motion_ramp.py
                   ramp_data.txt
           )

  # define the ramp_data fixture to chain tests
  set_tests_properties(motion::test_motion_ramp PROPERTIES FIXTURES_REQUIRED motion::ramp_data)
  set_tests_properties(motion::rampgen PROPERTIES FIXTURES_SETUP motion::ramp_data)
else()
  message(STATUS "pandas not available, skipping test motion::test_motion_ramp")
endif()
