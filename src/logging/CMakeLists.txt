target_sources(firmware PRIVATE log.cpp log_dest_rtt.cpp log_dest_shared.cpp log_platform.cpp)

if(BOARD MATCHES ".*BUDDY")
  target_sources(firmware PRIVATE log_dest_swo.cpp log_dest_syslog.cpp log_dest_usb.cpp)
endif()

if(BOARD STREQUAL "DWARF")
  target_sources(firmware PRIVATE log_dest_bufflog.cpp)
endif()
