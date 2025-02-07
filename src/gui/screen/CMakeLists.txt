add_subdirectory(filament)
add_subdirectory(toolhead)

target_sources(firmware PRIVATE screen_preheat.cpp)

if(HAS_CHAMBER_FILTRATION_API)
  target_sources(firmware PRIVATE screen_chamber_filtration.cpp)
endif()
