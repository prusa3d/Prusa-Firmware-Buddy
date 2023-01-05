target_include_directories(firmware PRIVATE include_${PRINTER})

target_sources(
  firmware
  PRIVATE gcode_description.cpp
          gui_media_events.cpp
          gui_time.cpp
          guimain.cpp
          IScreenPrinting.cpp
          logger.c
          menu_vars.cpp
          MItem_crash.cpp
          MItem_eeprom.cpp
          MItem_experimental_tools.cpp
          MItem_filament.cpp
          MItem_lan.cpp
          MItem_menus.cpp
          MItem_print.cpp
          MItem_tools.cpp
          network_gui_tools.cpp
          png_resources.cpp
          resource.c
          screen_crash_recovery.cpp
          screen_dialog_does_not_exist.cpp
          screen_filebrowser.cpp
          screen_hardfault.cpp
          screen_home.cpp
          screen_menu.cpp
          screen_menu_connect.cpp
          screen_menu_eeprom.cpp
          screen_menu_eeprom_diagnostics.cpp
          screen_menu_filament.cpp
          screen_menu_footer_settings.cpp
          screen_menu_fw_update.cpp
          screen_menu_hw_setup.cpp
          screen_menu_info.cpp
          screen_menu_lan_settings.cpp
          screen_menu_languages.cpp
          screen_menu_move.cpp
          screen_menu_network.cpp
          screen_menu_odometer.cpp
          screen_menu_sensor_info.cpp
          screen_menu_service.cpp
          screen_menu_settings.cpp
          screen_menu_steel_sheets.cpp
          screen_menu_temperature.cpp
          screen_menu_tune.cpp
          screen_messages.cpp
          screen_print_preview.cpp
          screen_printing.cpp
          screen_prusa_link.cpp
          screen_qr_error.cpp
          screen_reset_error.cpp
          screen_sheet_rename.cpp
          screen_splash.cpp
          screen_sysinf.cpp
          screen_watchdog.cpp
          ScreenFactory.cpp
          ScreenHandler.cpp
          ScreenPrintingModel.cpp
          test_display.cpp
          window_file_list.cpp
          window_filebrowser.cpp
          window_header.cpp
          window_print_progress.cpp
          window_temp_graph.cpp
          window_thumbnail.cpp
  )

if(HAS_SERIAL_PRINT)
  target_sources(firmware PRIVATE screen_printing_serial.cpp)
endif()

if(DEBUG)
  add_subdirectory(test)
  target_sources(firmware PRIVATE screen_menu_experimental_settings_debug.cpp)
else()
  target_sources(firmware PRIVATE screen_menu_experimental_settings_release.cpp)
endif()

if(HAS_SELFTEST)
  add_subdirectory(wizard)
  target_sources(
    firmware
    PRIVATE MItem_basic_selftest.cpp
            screen_menu_calibration.cpp
            # steel sheets requiere first layer calibration, which require wizard, which require
            # selftest TODO add HAS_STEEL_SHEETS property
            screen_menu_steel_sheets.cpp
            ScreenSelftest.cpp
            selftest_frame.cpp
    )
else()
  target_sources(firmware PRIVATE MItem_basic_selftest_disabled.cpp)
endif()

if(BOARD IN_LIST BOARDS_WITH_ST7789V)
  target_sources(firmware PRIVATE version_info_ST7789V.cpp)
endif()

add_subdirectory(dialogs)
add_subdirectory(footer)
