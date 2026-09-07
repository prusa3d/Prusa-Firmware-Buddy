if(BOARD_IS_MASTER_BOARD)
  target_sources(firmware PRIVATE calibration_preamble.cpp motion.cpp parking.cpp)
endif()

add_subdirectory(feedrates)
