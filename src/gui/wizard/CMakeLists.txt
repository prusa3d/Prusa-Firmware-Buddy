target_include_directories(firmware PRIVATE .)

target_sources(
  firmware
  PRIVATE firstlay.cpp
          screen_wizard.cpp
          selftest.cpp
          selftest_frame_esp.cpp
          selftest_frame_esp_progress.cpp
          selftest_frame_esp_qr.cpp
          selftest_invalid_state.cpp
          xyzcalib.cpp
  )
