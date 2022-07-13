target_sources(firmware PRIVATE cpu_utils.cpp)

if(BOARD MATCHES ".*BUDDY")
  target_sources(firmware PRIVATE buffered_serial.cpp)
endif()
