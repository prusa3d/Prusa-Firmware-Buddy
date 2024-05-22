target_include_directories(firmware PRIVATE include_${PRINTER})

target_sources(
  firmware
  PRIVATE i_selftest.cpp
          selftest_${PRINTER}.cpp
          selftest_axis.cpp
          selftest_axis_interface.cpp
          selftest_data.cpp
          selftest_esp_update.cpp
          selftest_fan.cpp
          selftest_fans_interface.cpp
          selftest_firstlayer.cpp
          selftest_firstlayer_interface.cpp
          selftest_heater.cpp
          selftest_heaters_interface.cpp
          selftest_netstatus_interface.cpp
          selftest_part.cpp
          selftest_tool_helper.cpp
  )

if(HAS_LOADCELL)
  target_sources(firmware PRIVATE selftest_loadcell.cpp selftest_loadcell_interface.cpp)
endif()

if(PRINTER STREQUAL "MK4")
  target_sources(firmware PRIVATE selftest_gears.cpp selftest_hotend_specify.cpp)
endif()

if(PRINTER STREQUAL "MK3.5")
  target_sources(firmware PRIVATE selftest_hotend_specify.cpp)
endif()

if(PRINTER STREQUAL "XL")
  target_sources(
    firmware PRIVATE selftest_nozzle_diameter.cpp selftest_nozzle_diameter_interface.cpp
    )
endif()

if(HAS_ADVANCED_POWER)
  target_sources(firmware PRIVATE power_check.cpp power_check_both.cpp)
else()
  target_sources(firmware PRIVATE power_check.cpp power_check_both_dummy.cpp)
endif()

if(FILAMENT_SENSOR STREQUAL ADC)
  target_sources(firmware PRIVATE selftest_fsensor.cpp selftest_fsensor_interface.cpp)
endif()

if(HAS_DWARF)
  target_sources(
    firmware PRIVATE selftest_dock.cpp selftest_dock_interface.cpp selftest_tool_offsets.cpp
                     selftest_tool_offsets_interface.cpp
    )
endif()
