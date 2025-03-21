target_sources(firmware PRIVATE menu_item_gcode_action.cpp menu_item_toggle_switch.cpp)

if(HAS_PRINT_FAN_TYPE)
  target_sources(firmware PRIVATE menu_item_select_menu.cpp)
endif()
