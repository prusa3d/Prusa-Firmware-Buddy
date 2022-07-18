target_sources(
  firmware
  PRIVATE DialogFactory.cpp
          DialogHandler.cpp
          DialogLoadUnload.cpp
          DialogMoveZ.cpp
          DialogRadioButton.cpp
          DialogStateful.cpp
          DialogTimed.cpp
          IDialog.cpp
          liveadjust_z.cpp
          window_dlg_calib_z.cpp
          window_dlg_load_unload.cpp
          window_dlg_popup.cpp
          window_dlg_preheat.cpp
          window_dlg_strong_warning.cpp
          window_dlg_wait.cpp
  )
