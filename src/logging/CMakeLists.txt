target_sources(firmware PRIVATE log.c log_dest_rtt.c log_dest_shared.c log_platform.c)

if(BOARD MATCHES ".*BUDDY")
  target_sources(firmware PRIVATE log_dest_swo.c log_dest_syslog.c log_dest_usb.c)
endif()

if(BOARD STREQUAL "DWARF")
  target_sources(firmware PRIVATE log_dest_bufflog.c)
endif()
