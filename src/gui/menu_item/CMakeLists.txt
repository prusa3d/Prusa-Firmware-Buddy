target_sources(
  firmware PRIVATE menu_item_color.cpp menu_item_gcode_action.cpp menu_item_select_menu.cpp
                   menu_item_toggle_switch.cpp menu_item_utils.cpp
  )

if(HAS_ANFC)
  target_sources(firmware PRIVATE menu_item_uuid.cpp)
endif()

if(HAS_CYPHAL)
  target_sources(firmware PRIVATE menu_item_cyphal_node_status.cpp)
endif()

add_subdirectory(specific)
