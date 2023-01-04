target_include_directories(firmware PRIVATE include_${PRINTER})

target_sources(
  firmware
  PRIVATE i_selftest.cpp
          selftest_${PRINTER}.cpp
          selftest_axis.cpp
          selftest_axis_interface.cpp
          selftest_esp_update.cpp
          selftest_fan.cpp
          selftest_fans_interface.cpp
          selftest_firstlayer.cpp
          selftest_firstlayer_interface.cpp
          selftest_heater.cpp
          selftest_heaters_interface.cpp
          selftest_netstatus_interface.cpp
          selftest_part.cpp
  )
