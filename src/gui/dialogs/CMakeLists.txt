target_include_directories(firmware PRIVATE .)

target_sources(
  firmware
  PRIVATE DialogConnectReg.cpp
          DialogHandler.cpp
          DialogLoadUnload.cpp
          DialogMoveZ.cpp
          DialogTimed.cpp
          i_radio_button.cpp
          IDialog.cpp
          liveadjust_z.cpp
          print_progress.cpp
          radio_button.cpp
          window_dlg_calib_z.cpp
          window_dlg_popup.cpp
          window_dlg_preheat.cpp
          window_dlg_quickpause.cpp
          window_dlg_wait.cpp
          window_dlg_warning.cpp
  )

if(BOARD STREQUAL "BUDDY")
  add_subdirectory(resolution_240x320)
endif()

if(BOARD STREQUAL "XBUDDY" OR BOARD STREQUAL "XLBUDDY")
  add_subdirectory(resolution_480x320)
endif()
