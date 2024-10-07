if(HAS_BELT_TUNING)
  add_subdirectory(belt_tuning)
endif()

if(HAS_XBUDDY_EXTENSION)
  add_subdirectory(xbuddy_extension)
endif()

if(HAS_CHAMBER_API)
  add_subdirectory(chamber)
endif()
