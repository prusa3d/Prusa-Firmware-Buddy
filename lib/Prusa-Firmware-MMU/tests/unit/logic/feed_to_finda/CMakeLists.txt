# define the test executable
add_executable(
  feed_to_finda_tests
  ${CMAKE_SOURCE_DIR}/src/logic/feed_to_finda.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/buttons.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/debouncer.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/finda.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/fsensor.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/globals.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/idler.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/leds.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/movable_base.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/permanent_storage.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/selector.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/user_input.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/pulse_gen.cpp
  ${MODULES_STUBS_DIR}/stub_adc.cpp
  ${MODULES_STUBS_DIR}/stub_eeprom.cpp
  ${MODULES_STUBS_DIR}/stub_gpio.cpp
  ${MODULES_STUBS_DIR}/stub_shr16.cpp
  ${MODULES_STUBS_DIR}/stub_timebase.cpp
  ${MODULES_STUBS_DIR}/stub_tmc2130.cpp
  ${LOGIC_STUBS_DIR}/main_loop_stub.cpp
  ${LOGIC_STUBS_DIR}/stub_motion.cpp
  test_feed_to_finda.cpp
  )

# define required search paths
target_include_directories(
  feed_to_finda_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/hal
                             ${CMAKE_SOURCE_DIR}/src/logic
  )

# tell build system about the test case
add_catch_test(feed_to_finda_tests)
