target_sources(marlin_server_types PUBLIC filament_change_phases.cpp safety_timer_phases.cpp)

if(HAS_INDX)
  target_sources(marlin_server_types PUBLIC nozzle_mismatch_phases.cpp)
endif()

if(HAS_GANTRY_SQUARENESS_CHECK)
  target_sources(marlin_server_types PUBLIC gantry_squareness_phases.cpp)
endif()

if(HAS_WASTEBIN_FILL_TRACKING)
  target_sources(marlin_server_types PUBLIC wastebin_emptying_phases.cpp)
endif()
