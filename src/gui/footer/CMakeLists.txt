target_include_directories(firmware PRIVATE .)

target_sources(
  firmware
  PRIVATE footer_icon.cpp
          footer_item_axis.cpp
          footer_item_fans.cpp
          footer_item_filament.cpp
          footer_item_fsensor.cpp
          footer_item_fsvalue.cpp
          footer_item_heatbreak.cpp
          footer_item_input_shaper.cpp
          footer_item_live_z.cpp
          footer_item_printspeed.cpp
          footer_items_heaters.cpp
          footer_items_nozzle_bed.cpp
          footer_line.cpp
          footer_text.cpp
          ifooter.cpp
          ifooter_item.cpp
          ifooter_string_item.cpp
  )

if(BOARD IN_LIST BOARDS_WITH_ILI9488)
  target_sources(firmware PRIVATE footer_singleline.cpp)
elseif(BOARD IN_LIST BOARDS_WITH_ST7789V)
  target_sources(firmware PRIVATE footer_doubleline.cpp)
endif()

if(HAS_MMU2 OR HAS_TOOLCHANGER)
  target_sources(firmware PRIVATE footer_item_multitool.cpp)
endif()

if(HAS_SHEET_PROFILES)
  target_sources(firmware PRIVATE footer_item_sheet_profile.cpp)
endif()

if(HAS_CHAMBER_API)
  target_sources(firmware PRIVATE footer_item_chamber_temp.cpp)
endif()
