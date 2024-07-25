target_sources(
  firmware PRIVATE screen_toolhead_settings.cpp screen_toolhead_settings_common.cpp
                   screen_toolhead_settings_fs.cpp
  )

if(HAS_TOOLCHANGER)
  target_sources(
    firmware PRIVATE screen_toolhead_settings_dock.cpp screen_toolhead_settings_nozzle_offset.cpp
    )
endif()
