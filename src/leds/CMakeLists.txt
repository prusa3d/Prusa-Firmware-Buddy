if(PRINTER STREQUAL "XL" AND BOARD MATCHES "BUDDY")
  target_sources(firmware PRIVATE side_strip.cpp side_strip_control.cpp task.cpp)
endif()
