target_include_directories(firmware PRIVATE . include)

target_link_libraries(firmware PRIVATE buddy_utils bsod random random_sw fanctl)

# Provides the FreeRTOS tick/idle hooks. Must be built wherever cpu_utils.cpp is (every board that
# enters this directory), as the hooks dispatch into it.
target_sources(firmware PRIVATE application_hooks.cpp)

if(BOARD IN_LIST BUDDY_BOARDS)
  target_sources(
    firmware
    PRIVATE adc.cpp
            all_tools_quick_cooling_manager.cpp
            app_metrics.cpp
            appmain.cpp
            awdg.cpp
            bbf.cpp
            conserve_cpu.cpp
            crc32.cpp
            data_exchange.cpp
            directory.cpp
            error_code_mangle.cpp
            extended_printer_type.cpp
            ExtInterruptHandler.cpp
            filament.cpp
            filament_common.cpp
            filament_list.cpp
            filament_presets.cpp
            filament_to_load.cpp
            filament_tools.cpp
            filename_type.cpp
            filepath_operation.cpp
            footer_def.cpp
            footer_eeprom.cpp
            fsm_states.cpp
            general_response.cpp
            hardware_rng.cpp
            hardware_serial.cpp
            heap.cpp
            hw_check.cpp
            hwio_pindef_check.cpp
            i2c.cpp
            ini_handler.cpp
            jsmn.cpp
            json_encode.cpp
            lang.cpp
            language_eeprom.cpp
            lfn.cpp
            marlin_client.cpp
            marlin_log_component.cpp
            marlin_print_preview.cpp
            marlin_server.cpp
            marlin_vars.cpp
            MarlinPin.cpp
            media_prefetch/media_prefetch.cpp
            media_prefetch/prefetch_compression.cpp
            media_prefetch_instance.cpp
            metric.cpp
            metric_handlers.cpp
            odometer.cpp
            otp.cpp
            path_utils.cpp
            Pin.cpp
            ping_manager.cpp
            print_utils.cpp
            printer_model.cpp
            probe_analysis.cpp
            random_hw.cpp
            ring_allocator.cpp
            rw_mutex.cpp
            safe_state.cpp
            segmented_json.cpp
            sensor_data.cpp
            sound.cpp
            spi_flash_bus.cpp
            st25dv64k.cpp
            stat_retry.cpp
            static_storage.cpp
            str_utils.cpp
            stubs.cpp
            support_utils.cpp
            support_utils_lib.cpp
            sys.cpp
            sys_time.cpp
            tasks.cpp
            tool_index.cpp
            tools_mapping.cpp
            w25x.cpp
            wdt.cpp
    )
  add_subdirectory(async_job)
  target_link_libraries(firmware PRIVATE raii)

  if(BOARD STREQUAL "XLBUDDY")
    target_sources(firmware PRIVATE hwio_XLBuddy.cpp)
  else()
    target_sources(firmware PRIVATE hwio_buddy_2209_02.cpp)
  endif()

  if(HAS_SHEET_PROFILES)
    target_sources(firmware PRIVATE SteelSheets.cpp)
  endif()
  if(HAS_ESP)
    target_sources(firmware PRIVATE fsm_network_setup.cpp)
  endif()
  if(HAS_E2EE_SUPPORT)
    add_subdirectory(e2ee)
  endif()
  if(HAS_SERIAL_PRINT)
    target_sources(firmware PRIVATE serial_printing.cpp)
  endif()
  if(HAS_INTERNAL_STORAGE_FLASH)
    target_sources(firmware PRIVATE mt29f_flash.cpp)
  endif()
elseif(BOARD STREQUAL "DWARF")
  target_sources(
    firmware
    PRIVATE adc.cpp
            heap.cpp
            marlin_log_component.cpp
            otp.cpp
            Pin.cpp
            random_sw.cpp
            safe_state.cpp
            tool_index.cpp
    )
  target_link_libraries(firmware PRIVATE logging)

elseif(BOARD STREQUAL "MODULARBED")
  target_sources(firmware PRIVATE heap.cpp otp.cpp)
  target_link_libraries(firmware PRIVATE logging)
elseif(BOARD STREQUAL "XL_DEV_KIT_XLB")
  target_sources(
    firmware
    PRIVATE adc.cpp
            app_metrics.cpp
            appmain.cpp
            crc32.cpp # todo: remove
            data_exchange.cpp
            ExtInterruptHandler.cpp
            fsm_states.cpp
            heap.cpp
            hwio_XLBuddy.cpp # todo: replace
            i2c.cpp
            marlin_client.cpp
            marlin_errors.cpp
            marlin_log_component.cpp
            marlin_print_preview.cpp
            marlin_server.cpp
            marlin_vars.cpp
            MarlinPin.cpp
            metric.cpp
            odometer.cpp
            otp.cpp
            Pin.cpp
            print_utils.cpp
            probe_analysis.cpp
            random_sw.cpp
            safe_state.cpp
            sound.cpp
            st25dv64k.cpp # todo: remove
            tasks.cpp
            tools_mapping.cpp
            wdt.cpp
    )
  if(HAS_SERIAL_PRINT)
    target_sources(firmware PRIVATE serial_printing.cpp)
  endif()
endif()

if(HAS_POWER_PANIC)
  target_sources(firmware PRIVATE power_panic.cpp)

  if(${POWER_PANIC_STORAGE_IS_FLASH})
    target_sources(firmware PRIVATE power_panic_storage_flash.cpp)
  elseif(${POWER_PANIC_STORAGE_IS_BKPSRAM})
    target_sources(firmware PRIVATE power_panic_storage_bkpsram.cpp)
  else()
    message(FATAL_ERROR "POWER_PANIC_STORAGE storage is unknown")
  endif()
endif()

if(HAS_LOADCELL)
  target_sources(firmware PRIVATE loadcell.cpp position_lookback.cpp)
endif()
if(HAS_LOADCELL_HX717)
  target_sources(firmware PRIVATE hx717.cpp)
endif()
if(HAS_LOADCELL AND HAS_LOADCELL_HX717)
  target_sources(firmware PRIVATE hx717mux.cpp)
endif()

if(GUI)
  # TODO: Some of it should be moved under GUI
  target_sources(firmware PRIVATE bsod_gui.cpp filament_gui.cpp)
endif()

if(BOARD IN_LIST BUDDY_BOARDS)
  if(HAS_SELFTEST)
    target_sources(firmware PRIVATE selftest_result_type.cpp)
    add_subdirectory(selftest)
  endif()
endif()

if(HAS_ADVANCED_POWER)
  target_sources(firmware PRIVATE advanced_power.cpp)
endif()

if(HAS_NFC)
  target_sources(firmware PRIVATE nfc.cpp)
endif()

if(HAS_HOTEND_TYPE_SUPPORT)
  target_sources(firmware PRIVATE hotend_type.cpp)
endif()

if(HAS_TOOL_MAPPING)
  target_sources(firmware PRIVATE multi_filament_change.cpp)
endif()

add_subdirectory(fanctl)
add_subdirectory(marlin_server_types)
add_subdirectory(printer_variant)

if(BOARD IN_LIST BUDDY_BOARDS)
  add_subdirectory(crash_dump)
  add_subdirectory(automata)
  add_subdirectory(http)
endif()
if(BOARD_IS_MASTER_BOARD)
  add_subdirectory(gcode)
  add_subdirectory(mapi)
endif()

if(BOARD_IS_MASTER_BOARD OR (BOARD STREQUAL "DWARF"))
  add_subdirectory(tool)
endif()

add_subdirectory(utils)
