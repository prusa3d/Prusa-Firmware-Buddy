target_sources(firmware PRIVATE median_filter.cpp)

if(BOARD_IS_MASTER_BOARD)
  target_sources(firmware PRIVATE kalman.cpp)
endif()
