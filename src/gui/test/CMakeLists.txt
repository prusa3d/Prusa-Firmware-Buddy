target_sources(
  firmware PRIVATE screen_menu_test.cpp screen_mesh_bed_lv.cpp screen_test_gui.cpp
                   screen_test_msgbox.cpp screen_test_term.cpp screen_test_wizard_icons.cpp
  )

if(FILAMENT_SENSOR STREQUAL "HX711")
  target_sources(firmware PRIVATE screen_test_HX711.cpp)
endif()
