add_executable(
  xbuddy_extension_modbus_tests ${CMAKE_SOURCE_DIR}/src/puppy/xbuddy_extension/modbus.cpp
                                modbus_tests.cpp
  )
target_include_directories(xbuddy_extension_modbus_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_catch_test(xbuddy_extension_modbus_tests)
