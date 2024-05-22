if(HAS_SIDE_LEDS AND BOARD MATCHES "BUDDY")
  target_sources(firmware PRIVATE side_strip.cpp side_strip_control.cpp)
endif()
