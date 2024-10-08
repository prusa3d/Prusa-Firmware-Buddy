target_sources(firmware PRIVATE menu_item_gcode_action.cpp menu_item_toggle_switch.cpp)

if(HAS_CHAMBER_API)
  target_sources(firmware PRIVATE menu_item_chamber.cpp)
endif()
