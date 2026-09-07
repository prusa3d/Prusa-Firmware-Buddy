target_include_directories(firmware PRIVATE .)
target_link_libraries(firmware PRIVATE font_data)

if(HAS_SELFTEST)
  add_subdirectory(include_${PRINTER})
endif()

target_sources(
  firmware
  PRIVATE auto_layout.cpp
          file_sort.cpp
          fonts.cpp
          frame_calibration_common.cpp
          frame_qr_layout.cpp
          gcode_description.cpp
          gui_media_events.cpp
          gui_time.cpp
          gui_utils.cpp
          guimain.cpp
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
          qr.cpp
          screen_error.cpp
          screen_filebrowser.cpp
          screen_help_fw_update.cpp
          screen_home.cpp
          screen_menu.cpp
          screen_menu_control.cpp
          screen_menu_enclosure.cpp
          screen_menu_experimental_settings.cpp
          screen_menu_fail_stat.cpp
          screen_menu_fan_info.cpp
          screen_menu_filament.cpp
          screen_menu_footer_settings.cpp
          screen_menu_hardware.cpp
          screen_menu_hardware_tune.cpp
          screen_menu_info.cpp
          screen_menu_input_shaper.cpp
          screen_menu_lang_and_time.cpp
          screen_menu_metrics.cpp
          screen_menu_metrics_list.cpp
          screen_menu_move.cpp
          screen_menu_network.cpp
          screen_menu_network_settings.cpp
          screen_menu_network_status.cpp
          screen_menu_sensor_info.cpp
          screen_menu_settings.cpp
          screen_menu_statistics.cpp
          screen_menu_system.cpp
          screen_menu_temperature_and_fans.cpp
          screen_menu_tune.cpp
          screen_menu_user_interface.cpp
          screen_messages.cpp
          screen_move_z.cpp
          screen_printer_setup.cpp
          screen_printing.cpp
          screen_printing_end_result.cpp
          screen_prusa_link.cpp
          screen_splash.cpp
          ScreenFactory.cpp
          ScreenHandler.cpp
          ScreenPrintingModel.cpp
          text_error_url.cpp
          time_tools.cpp
          window_file_list.cpp
          window_filebrowser.cpp
          window_header.cpp
          window_print_progress.cpp
          window_thumbnail.cpp
          window_wizard_icon.cpp
  )

if(HAS_CRASH_DETECTION)
  target_sources(firmware PRIVATE screen_crash_recovery.cpp)
endif()

if(HAS_SERIAL_PRINT)
  target_sources(firmware PRIVATE screen_printing_serial.cpp)
endif()

if(TRANSLATIONS_ENABLED)
  target_sources(firmware PRIVATE screen_menu_languages.cpp)
endif()

if(HAS_XLCD_TOUCH_DRIVER)
  target_sources(firmware PRIVATE MItem_touch.cpp)
  if(DEVELOPMENT_ITEMS_ENABLED)
    target_sources(firmware PRIVATE screen_touch_playground.cpp)
  endif()
endif()

if(DEVELOPMENT_ITEMS_ENABLED)
  target_sources(
    firmware PRIVATE MItem_development.cpp screen_menu_advanced_footer_settings.cpp
                     screen_menu_development.cpp screen_menu_error_test.cpp
    )
endif()

if(HAS_TOOLCHANGER)
  target_sources(firmware PRIVATE screen_menu_filament_changeall.cpp)
endif()

if(HAS_MODULAR_BED)
  target_sources(firmware PRIVATE screen_menu_modular_bed.cpp)
endif()

if(HAS_REMOTE_BED)
  target_sources(firmware PRIVATE screen_menu_remote_bed.cpp)
endif()

if(HAS_SELFTEST)
  add_subdirectory(wizard)
  target_sources(
    firmware PRIVATE screen_menu_selftest_snake.cpp ScreenSelftest.cpp selftest_action_helpers.cpp
                     selftest_dependencies.cpp selftest_frame.cpp
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
    )
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
  target_sources(
    firmware PRIVATE display_hw_checks_xlcd.cpp screen_menu_board_info.cpp version_info_ILI9488.cpp
    )
endif()

if(XL_ENCLOSURE_SUPPORT)
  target_sources(firmware PRIVATE MItem_enclosure.cpp screen_change_filter.cpp)
endif()

if(HAS_FILAMENT_SENSORS_MENU)
  target_sources(firmware PRIVATE screen_menu_filament_sensors.cpp)
endif()

if(HAS_COLDPULL)
  target_sources(firmware PRIVATE screen_cold_pull.cpp)
endif()

if(HAS_PHASE_STEPPING)
  target_sources(firmware PRIVATE screen_menu_phase_stepping.cpp)
endif()

if(HAS_PHASE_STEPPING_CALIBRATION)
  target_sources(firmware PRIVATE screen_phase_stepping_calibration.cpp)
endif()
if(HAS_E2EE_SUPPORT)
  target_sources(firmware PRIVATE screen_menu_e2ee.cpp)
endif()

if(CONNECT)
  target_sources(firmware PRIVATE screen_menu_connect.cpp)
endif()

if(HAS_ESP)
  target_sources(firmware PRIVATE screen_network_setup.cpp)
endif()

if(HAS_INPUT_SHAPER_CALIBRATION)
  target_sources(firmware PRIVATE screen_input_shaper_calibration.cpp)
endif()

add_subdirectory(dialogs)
add_subdirectory(footer)
add_subdirectory(screen)
add_subdirectory(menu_item)
add_subdirectory(standard_frame)

if(HAS_BED_LEVEL_CORRECTION)
  target_sources(firmware PRIVATE screen_menu_bed_level_correction.cpp)
endif()
