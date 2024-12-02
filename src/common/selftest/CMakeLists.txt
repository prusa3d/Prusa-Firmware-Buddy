target_include_directories(Marlin_Config INTERFACE include_${PRINTER})

target_sources(
  firmware
  PRIVATE i_selftest.cpp
          power_check.cpp
          selftest_${PRINTER}.cpp
          selftest_axis.cpp
          selftest_axis_interface.cpp
          selftest_data.cpp
          selftest_fans.cpp
          selftest_heater.cpp
          selftest_heaters_interface.cpp
          selftest_part.cpp
          selftest_revise_printer_setup.cpp
          selftest_tool_helper.cpp
  )

if(CONNECT OR WUI)
  target_sources(firmware PRIVATE selftest_netstatus_interface.cpp)
endif()

if(HAS_LOADCELL)
  target_sources(firmware PRIVATE selftest_loadcell.cpp selftest_loadcell_interface.cpp)
endif()

if(PRINTER STREQUAL "MK4")
  target_sources(firmware PRIVATE selftest_gears.cpp)
endif()

if(PRINTER STREQUAL "MINI" OR PRINTER STREQUAL "MK3.5")
  target_sources(firmware PRIVATE selftest_firstlayer.cpp selftest_firstlayer_interface.cpp)
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
