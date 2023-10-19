# define the test executable
add_executable(
  tool_change_tests
  ${CMAKE_SOURCE_DIR}/src/application.cpp
  ${CMAKE_SOURCE_DIR}/src/registers.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/command_base.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/cut_filament.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/eject_filament.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/feed_to_finda.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/feed_to_bondtech.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/feed_to_finda.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/home.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/load_filament.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/move_selector.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/no_command.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/retract_from_finda.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/set_mode.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/start_up.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/tool_change.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/unload_filament.cpp
  ${CMAKE_SOURCE_DIR}/src/logic/unload_to_finda.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/buttons.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/debouncer.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/finda.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/fsensor.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/globals.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/idler.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/leds.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/movable_base.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/permanent_storage.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/protocol.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/pulley.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/selector.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/user_input.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/pulse_gen.cpp
  ${CMAKE_SOURCE_DIR}/src/modules/voltage.cpp
  ${MODULES_STUBS_DIR}/stub_adc.cpp
  ${MODULES_STUBS_DIR}/stub_cpu.cpp
  ${MODULES_STUBS_DIR}/stub_eeprom.cpp
  ${MODULES_STUBS_DIR}/stub_gpio.cpp
  ${MODULES_STUBS_DIR}/stub_shr16.cpp
  ${MODULES_STUBS_DIR}/stub_serial.cpp
  ${MODULES_STUBS_DIR}/stub_timebase.cpp
  ${MODULES_STUBS_DIR}/stub_tmc2130.cpp
  ${LOGIC_STUBS_DIR}/homing.cpp
  ${LOGIC_STUBS_DIR}/main_loop_stub.cpp
  ${LOGIC_STUBS_DIR}/stub_motion.cpp
  test_tool_change.cpp
  )

# define required search paths
target_include_directories(
  tool_change_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/hal
                           ${CMAKE_SOURCE_DIR}/src/logic
  )

# tell build system about the test case
add_catch_test(tool_change_tests)
