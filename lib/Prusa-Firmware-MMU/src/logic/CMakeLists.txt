target_sources(
  firmware
  PRIVATE command_base.cpp
          cut_filament.cpp
          eject_filament.cpp
          feed_to_bondtech.cpp
          feed_to_finda.cpp
          home.cpp
          load_filament.cpp
          no_command.cpp
          retract_from_finda.cpp
          set_mode.cpp
          tool_change.cpp
          unload_filament.cpp
          unload_to_finda.cpp
  )
