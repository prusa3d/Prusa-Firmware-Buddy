target_include_directories(firmware PRIVATE ${PRINTER})

target_sources(
  firmware
  PRIVATE selftest_esp_update.cpp
          selftest_axis_interface.cpp
          selftest_fans_interface.cpp
          selftest_heaters_interface.cpp
          selftest_netstatus_interface.cpp
          i_selftest.cpp
          selftest_part.cpp
          selftest_fan.cpp
          selftest_axis.cpp
          selftest_heater.cpp
          selftest_${PRINTER}.cpp
  )
