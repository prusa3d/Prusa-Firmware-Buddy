target_sources(firmware PRIVATE safety_timer.cpp)

if(HAS_CHAMBER_API)
  add_subdirectory(chamber)
endif()
