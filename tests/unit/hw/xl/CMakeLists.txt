add_executable(
  modular_bed_fan_tests ${CMAKE_SOURCE_DIR}/src/hw/xl/modular_bed_fan.cpp modular_bed_fan_tests.cpp
  )
target_include_directories(
  modular_bed_fan_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/hw/xl ${CMAKE_SOURCE_DIR}/src/common
  )
add_catch_test(modular_bed_fan_tests)
