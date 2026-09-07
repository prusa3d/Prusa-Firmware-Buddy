target_sources(
  firmware PRIVATE screen_crash_dump.cpp screen_initial_network_setup.cpp
                   screen_print_readiness.cpp screen_welcome.cpp
  )

if(HAS_EMERGENCY_STOP)
  target_sources(firmware PRIVATE screen_emergency_stop_consent.cpp)
endif()

if(HAS_HEATBED_SCREWS_DURING_TRANSPORT)
  target_sources(firmware PRIVATE screen_remove_heatbed_screws.cpp)
endif()

if(HAS_HT_HOTEND)
  target_sources(firmware PRIVATE screen_hotend_type_changed.cpp)
endif()

if(HAS_XLCD_TOUCH_DRIVER)
  target_sources(firmware PRIVATE screen_touch_driver_failed.cpp)
endif()
