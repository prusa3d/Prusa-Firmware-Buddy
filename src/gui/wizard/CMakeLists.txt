target_include_directories(firmware PRIVATE .)

target_sources(
  firmware
  PRIVATE selftest_frame_axis.cpp
          selftest_frame_esp.cpp
          selftest_frame_esp_progress.cpp
          selftest_frame_esp_qr.cpp
          selftest_frame_fans.cpp
          selftest_frame_result.cpp
          selftest_frame_temp.cpp
          selftest_frame_wizard_epilogue.cpp
          selftest_frame_wizard_prologue.cpp
          selftest_group.cpp
          selftest_invalid_state.cpp
          selftest_result_axis.cpp
          selftest_result_eth.cpp
          selftest_result_fans.cpp
          selftest_result_heaters.cpp
          selftest_result_wifi.cpp
          selftest_view.cpp
          selftest_view_item.cpp
          selftest_view_item_separator.cpp
          selftest_view_item_text.cpp
          test_of_selftest_result.cpp
  )
