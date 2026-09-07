add_subdirectory(filament)
add_subdirectory(initial)
add_subdirectory(toolhead)
add_subdirectory(print)

target_sources(
  firmware
  PRIVATE dialog_safety_timer.cpp
          screen_factory_reset.cpp
          screen_filament_change.cpp
          screen_m600.cpp
          screen_menu_virtual.cpp
          screen_preheat.cpp
          screen_print_preview.cpp
          screen_printer_type_changed.cpp
  )

if(HAS_ANFC)
  add_subdirectory(openprinttag)
endif()

if(HAS_LOADCELL)
  target_sources(firmware PRIVATE screen_nozzle_cleaning_failed.cpp)
endif()

if(HAS_CHAMBER_FILTRATION_API)
  target_sources(firmware PRIVATE screen_chamber_filtration.cpp)
endif()

if(HAS_CANCEL_OBJECT)
  target_sources(firmware PRIVATE screen_cancel_objects.cpp)
endif()

if(HAS_MMU2)
  target_sources(firmware PRIVATE screen_hw_mmu.cpp screen_menu_mmu_preload_all.cpp)
endif()

if(HAS_LIGHTS_MENU)
  target_sources(firmware PRIVATE screen_menu_lights.cpp)
endif()

if(HAS_SELFTEST)
  add_subdirectory(selftest)
endif()

if(HAS_TOOLCHANGER)
  target_sources(firmware PRIVATE screen_tool_pick_park.cpp)
endif()

if(HAS_INDX)
  target_sources(firmware PRIVATE screen_nozzle_mismatch.cpp)
endif()
