target_sources(firmware PRIVATE median_filter.cpp)

if(BOARD MATCHES ".*BUDDY")
  target_sources(firmware PRIVATE kalman.cpp)
endif()
