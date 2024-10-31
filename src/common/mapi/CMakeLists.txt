if(BOARD_IS_MASTER_BOARD)
  target_sources(firmware PRIVATE motion.cpp parking.cpp)
endif()
