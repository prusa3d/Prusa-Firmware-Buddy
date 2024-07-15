target_include_directories(firmware PRIVATE .)
add_subdirectory(include_${PRINTER})

if(BOARD STREQUAL "BUDDY")
  add_subdirectory(resolution_240x320)
endif()

if(BOARD STREQUAL "XBUDDY" OR BOARD STREQUAL "XLBUDDY")
  add_subdirectory(resolution_480x320)
endif()

target_sources(
  firmware
  PRIVATE box_unfinished_selftest.cpp
          file_sort.cpp
          fonts.cpp
          frame_qr_layout.cpp
          gcode_description.cpp
          gui_bootstrap_screen.cpp
          gui_media_events.cpp
          gui_time.cpp
          guimain.cpp
          img_resources.cpp
          img_resources.cpp
          IScreenPrinting.cpp
          lazyfilelist.cpp
          logger.cpp
          menu_vars.cpp
          MItem_crash.cpp
          MItem_experimental_tools.cpp
          MItem_filament.cpp
          MItem_hardware.cpp
          MItem_input_shaper.cpp
          MItem_menus.cpp
          MItem_network.cpp
          MItem_print.cpp
          MItem_tools.cpp
          numeric_input_config_common.cpp
          print_time_module.cpp
          pseudo_screen_callback.cpp
          qoi_decoder.cpp
          screen_blue_error.cpp
          screen_bsod.cpp
          screen_change_filter.cpp
          screen_dialog_does_not_exist.cpp
          screen_filebrowser.cpp
          screen_hardfault.cpp
          screen_home.cpp
          screen_menu.cpp
          screen_menu_cancel_object.cpp
          screen_menu_control.cpp
          screen_menu_eeprom.cpp
          screen_menu_enclosure.cpp
          screen_menu_error_test.cpp
          screen_menu_factory_reset.cpp
          screen_menu_fail_stat.cpp
          screen_menu_filament.cpp
          screen_menu_footer_settings.cpp
          screen_menu_fw_update.cpp
          screen_menu_hardware.cpp
          screen_menu_hardware_tune.cpp
          screen_menu_info.cpp
          screen_menu_input_shaper.cpp
          screen_menu_lang_and_time.cpp
          screen_menu_metrics.cpp
          screen_menu_metrics_list.cpp
          screen_menu_move.cpp
          screen_menu_move_utils.cpp
          screen_menu_network.cpp
          screen_menu_network_settings.cpp
          screen_menu_network_status.cpp
          screen_menu_odometer.cpp
          screen_menu_sensor_info_${PRINTER}.cpp
          screen_menu_settings.cpp
          screen_menu_statistics.cpp
          screen_menu_system.cpp
          screen_menu_temperature.cpp
          screen_menu_tune.cpp
          screen_menu_user_interface.cpp
          screen_messages.cpp
          screen_network_setup.cpp
          screen_print_preview.cpp
          screen_printer_setup.cpp
          screen_printing.cpp
          screen_printing_end_result.cpp
          screen_prusa_link.cpp
          screen_qr_error.cpp
          screen_reset_error.cpp
          screen_splash.cpp
          screen_stack_overflow.cpp
          screen_sysinf.cpp
          screen_watchdog.cpp
          ScreenFactory.cpp
          ScreenHandler.cpp
          ScreenPrintingModel.cpp
          time_tools.cpp
          window_file_list.cpp
          window_filebrowser.cpp
          window_header.cpp
          window_msgbox_wrong_printer.cpp
          window_print_progress.cpp
          window_thumbnail.cpp
  )
if(PRINTER IN_LIST PRINTERS_WITH_CRASH_DETECTION)
  target_sources(firmware PRIVATE screen_crash_recovery.cpp)
endif()

if(HAS_SERIAL_PRINT)
  target_sources(firmware PRIVATE screen_printing_serial.cpp)
endif()

if(TRANSLATIONS_ENABLED)
  target_sources(firmware PRIVATE screen_menu_languages.cpp)
endif()

if(HAS_XLCD_TOUCH_DRIVER)
  target_sources(firmware PRIVATE MItem_touch.cpp screen_touch_playground.cpp)
endif()

if(HAS_TOOLCHANGER)
  target_sources(
    firmware PRIVATE screen_menu_filament_changeall.cpp screen_menu_tools.cpp
                     screen_tools_mapping.cpp
    )
else()
  target_sources(firmware PRIVATE screen_menu_no_tools.cpp)
endif()

if(HAS_MODULARBED)
  target_sources(firmware PRIVATE screen_menu_modularbed.cpp)
endif()

if(DEBUG)
  target_sources(firmware PRIVATE screen_menu_experimental_settings_debug.cpp)
else()
  target_sources(firmware PRIVATE screen_menu_experimental_settings_release.cpp)
endif()

if(HAS_SELFTEST)
  add_subdirectory(wizard)
  target_sources(
    firmware PRIVATE MItem_basic_selftest.cpp screen_menu_selftest_snake.cpp ScreenSelftest.cpp
                     selftest_frame.cpp
    )
endif()

if(HAS_LOADCELL)
  target_sources(firmware PRIVATE MItem_loadcell.cpp)
endif()

if(HAS_SHEET_PROFILES)
  target_sources(firmware PRIVATE MItem_steel_sheets.cpp screen_menu_steel_sheets.cpp)
endif()

if(HAS_MMU2)
  target_sources(
    firmware
    PRIVATE MItem_mmu.cpp
            screen_menu_filament_changeall.cpp
            screen_menu_filament_mmu.cpp
            screen_menu_mmu_cut_filament.cpp
            screen_menu_mmu_eject_filament.cpp
            screen_menu_mmu_load_test_filament.cpp
            screen_menu_mmu_load_to_nozzle.cpp
            screen_menu_mmu_preload_to_mmu.cpp
            screen_tools_mapping.cpp
    )
endif()

if(PRINTER STREQUAL "MK4"
   OR PRINTER STREQUAL "MK3.5"
   OR PRINTER STREQUAL "XL"
   OR PRINTER STREQUAL "MINI"
   )
  target_sources(firmware PRIVATE screen_help_fw_update.cpp)
endif()

if(PRINTER STREQUAL "MK4" OR PRINTER STREQUAL "MK3.5")
  target_sources(firmware PRIVATE screen_fatal_warning.cpp)
endif()

if(PRINTER STREQUAL "MINI")
  target_sources(firmware PRIVATE MItem_MINI.cpp)
endif()

if(PRINTER STREQUAL "MK3.5")
  target_sources(firmware PRIVATE MItem_MK3.5.cpp)
endif()

if(BOARD IN_LIST BOARDS_WITH_ST7789V)
  target_sources(firmware PRIVATE display_hw_checks_MINI.cpp version_info_ST7789V.cpp)
elseif(BOARD IN_LIST BOARDS_WITH_ILI9488)
  target_sources(firmware PRIVATE display_hw_checks_xlcd.cpp version_info_ILI9488.cpp)
endif()

if(BOARD STREQUAL "XBUDDY" OR BOARD STREQUAL "XLBUDDY")
  target_sources(firmware PRIVATE menu_item_xlcd.cpp)
endif()

if(BOARD STREQUAL "XLBUDDY")
  target_sources(firmware PRIVATE MItem_enclosure.cpp)
endif()

if(PRINTER IN_LIST PRINTERS_WITH_LOVE_BOARD)
  target_sources(firmware PRIVATE MItem_love_board.cpp)
endif()

if(HAS_FILAMENT_SENSORS_MENU)
  target_sources(firmware PRIVATE screen_menu_filament_sensors.cpp)
endif()

if(HAS_COLDPULL)
  target_sources(firmware PRIVATE screen_cold_pull.cpp)
endif()

if(HAS_PHASE_STEPPING)
  target_sources(firmware PRIVATE screen_phase_stepping.cpp)
endif()

if(CONNECT)
  target_sources(firmware PRIVATE screen_menu_connect.cpp)
endif()

if(HAS_INPUT_SHAPER_CALIBRATION)
  target_sources(firmware PRIVATE screen_input_shaper_calibration.cpp)
endif()

add_subdirectory(dialogs)
add_subdirectory(footer)
if(HAS_LEDS)
  add_subdirectory(led_animations)
endif()

if(PRINTER IN_LIST PRINTERS_WITH_BED_LEVEL_CORRECTION)
  target_sources(firmware PRIVATE screen_menu_bed_level_correction.cpp)
endif()
