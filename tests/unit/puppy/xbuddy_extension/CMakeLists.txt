add_executable(
  xbuddy_extension_modbus_tests
  ${CMAKE_SOURCE_DIR}/src/puppy/xbuddy_extension/modbus.cpp
  ${CMAKE_SOURCE_DIR}/src/puppy/xbuddy_extension/mmu.cpp
  ${CMAKE_SOURCE_DIR}/lib/Prusa-Firmware-MMU/src/modules/protocol.cpp
  modbus_tests.cpp
  modbus-mmu_tests.cpp
  )
target_include_directories(
  xbuddy_extension_modbus_tests PRIVATE include ${CMAKE_SOURCE_DIR}/src
                                        ${CMAKE_SOURCE_DIR}/src/puppy/xbuddy_extension_shared
  )
add_catch_test(xbuddy_extension_modbus_tests)
