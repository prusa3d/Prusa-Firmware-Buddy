add_subdirectory(fsensor)

if(PRINTER STREQUAL "COREONE_INDX")
  target_include_directories(Marlin_Config INTERFACE include_COREONE)
  target_sources(firmware PRIVATE selftest_COREONE.cpp)
elseif(PRINTER STREQUAL "COREONEL_INDX")
  target_include_directories(Marlin_Config INTERFACE include_COREONEL)
  target_sources(firmware PRIVATE selftest_COREONEL.cpp)
else()
  target_include_directories(Marlin_Config INTERFACE include_${PRINTER})
  target_sources(firmware PRIVATE selftest_${PRINTER}.cpp)
endif()

target_sources(
  firmware
  PRIVATE i_selftest.cpp
          power_check.cpp
          selftest_axis.cpp
          selftest_axis_interface.cpp
          selftest_data.cpp
          selftest_fans.cpp
          selftest_heater.cpp
          selftest_heaters_interface.cpp
          selftest_invocation.cpp
          selftest_part.cpp
          selftest_result_evaluation.cpp
          selftest_revise_printer_setup.cpp
          selftest_tool_helper.cpp
  )

if(HAS_HEATERS_SELFTEST_GCODE)
  target_sources(firmware PRIVATE heaters_selftest.cpp)
endif()

if(CONNECT OR WUI)
  target_sources(firmware PRIVATE selftest_netstatus_interface.cpp)
endif()

if(HAS_LOADCELL)
  if(HAS_INDX)
    target_sources(firmware PRIVATE selftest_loadcell_indx.cpp selftest_loadcell_interface_indx.cpp)
  else()
    target_sources(firmware PRIVATE selftest_loadcell.cpp selftest_loadcell_interface.cpp)
  endif()
endif()

if(PRINTER STREQUAL "MINI" OR PRINTER STREQUAL "MK3.5")
  target_sources(firmware PRIVATE selftest_firstlayer.cpp selftest_firstlayer_interface.cpp)
endif()

if(HAS_DWARF)
  target_sources(
    firmware PRIVATE selftest_dock.cpp selftest_dock_interface.cpp selftest_tool_offsets.cpp
                     selftest_tool_offsets_interface.cpp
    )
endif()
