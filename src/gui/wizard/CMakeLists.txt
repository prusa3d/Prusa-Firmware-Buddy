target_include_directories(firmware PRIVATE .)

target_sources(
  firmware
  PRIVATE selftest_frame_axis.cpp
          selftest_frame_calib_z.cpp
          selftest_frame_fans.cpp
          selftest_frame_firstlayer.cpp
          selftest_frame_firstlayer_questions.cpp
          selftest_frame_result.cpp
          selftest_frame_revise_printer_setup.cpp
          selftest_frame_temp.cpp
          selftest_frame_wizard_epilogue.cpp
          selftest_frame_wizard_prologue.cpp
          selftest_group.cpp
          selftest_invalid_state.cpp
          selftest_result_axis.cpp
          selftest_result_eth.cpp
          selftest_result_fans.cpp
          selftest_result_heaters.cpp
          selftest_view.cpp
          selftest_view_item.cpp
          selftest_view_item_separator.cpp
          selftest_view_item_text.cpp
          test_of_selftest_result.cpp
          window_wizard_icon.cpp
  )

if(NOT (${PRINTER} STREQUAL "MINI"))
  target_sources(
    firmware PRIVATE selftest_frame_fsensor.cpp selftest_frame_loadcell.cpp
                     selftest_result_fsensor.cpp selftest_result_loadcell.cpp
    )
endif()

if(PRINTER STREQUAL "MK4")
  target_sources(firmware PRIVATE selftest_frame_gears_calib.cpp)
endif()

if(${PRINTER} STREQUAL "XL")
  target_sources(firmware PRIVATE selftest_frame_dock.cpp selftest_frame_tool_offsets.cpp)
endif()

if(HAS_BELT_TUNING)
  target_sources(firmware PRIVATE screen_belt_tuning_wizard.cpp)
endif()
