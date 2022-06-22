target_include_directories(firmware PRIVATE ${PRINTER})

target_sources(
  firmware PRIVATE selftest_esp_update.cpp selftest_fan.cpp selftest_axis.cpp selftest_heater.cpp
                   selftest_${PRINTER}.cpp
  )
