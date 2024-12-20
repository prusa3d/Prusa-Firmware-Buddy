target_include_directories(firmware PRIVATE .)

target_sources(
  firmware
  PRIVATE dialog_text_input.cpp
          DialogHandler.cpp
          DialogLoadUnload.cpp
          DialogTimed.cpp
          i_radio_button.cpp
          IDialog.cpp
          liveadjust_z.cpp
          print_progress.cpp
          radio_button.cpp
          radio_button_fsm.cpp
          window_dlg_quickpause.cpp
          window_dlg_wait.cpp
          window_dlg_warning.cpp
  )

if(CONNECT)
  target_sources(firmware PRIVATE DialogConnectReg.cpp)
endif()

if(BOARD STREQUAL "BUDDY")
  add_subdirectory(resolution_240x320)
  target_sources(firmware PRIVATE)
endif()

if(BOARD STREQUAL "XBUDDY" OR BOARD STREQUAL "XLBUDDY")
  add_subdirectory(resolution_480x320)
  target_sources(firmware PRIVATE dialog_numeric_input.cpp)
endif()
