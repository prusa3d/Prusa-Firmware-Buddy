if(HAS_CHAMBER_API)
  target_sources(firmware PRIVATE menu_items_chamber.cpp)
endif()

if(HAS_CHAMBER_FILTRATION_API)
  target_sources(firmware PRIVATE menu_items_chamber_filtration.cpp)
endif()

if(HAS_XBUDDY_EXTENSION)
  target_sources(firmware PRIVATE menu_items_xbuddy_extension.cpp)
endif()
