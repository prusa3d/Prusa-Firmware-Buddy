target_include_directories(firmware PRIVATE . include)

if(BOARD MATCHES ".*BUDDY")
  target_sources(firmware PRIVATE calibrated_loveboard.cpp)
endif()

if(BOARD MATCHES ".*BUDDY")
  target_sources(
    firmware
    PRIVATE adc.cpp
            app_metrics.cpp
            appmain.cpp
            base64_stream_decoder.cpp
            basename.cpp
            bbf.cpp
            client_response_texts.cpp
            crc32.cpp
            crc32_zlib.cpp
            eeprom.cpp
            eeprom_loadsave.cpp
            ExtInterruptHandler.cpp
            filename_type.cpp
            footer_eeprom.cpp
            freertos_mutex.cpp
            fsm_types.cpp
            gcode_file.cpp
            gcode_filter.cpp
            gcode_info.cpp
            gcode_thumb_decoder.cpp
            hardware_serial.cpp
            heap.cpp
            hwio_pindef_check.cpp
            hx717.cpp
            i2c.cpp
            ini_handler.cpp
            jsmn.cpp
            json_encode.cpp
            lang.cpp
            language_eeprom.cpp
            lfn.cpp
            marlin_client.cpp
            marlin_errors.cpp
            marlin_events.cpp
            marlin_log_component.cpp
            marlin_move.cpp
            marlin_print_preview.cpp
            marlin_server.cpp
            marlin_vars.cpp
            MarlinPin.cpp
            media.cpp
            metric.cpp
            metric_handlers.cpp
            non_file_printing_counter.cpp
            odometer.cpp
            otp.cpp
            path_utils.cpp
            PersistentStorage.cpp
            Pin.cpp
            print_processor.cpp
            print_utils.cpp
            probe_analysis.cpp
            probe_position_lookback.cpp
            safe_state.cpp
            scratch_buffer.cpp
            segmented_json.cpp
            selftest_result_type.cpp
            sensor_data_buffer.cpp
            sound.cpp
            st25dv64k.cpp
            SteelSheets.cpp
            str_utils.cpp
            stubs.cpp
            support_utils.cpp
            support_utils_lib.cpp
            sys.cpp
            sys_time.cpp
            tasks.cpp
            time_tools.cpp
            timing.cpp
            trinamic.cpp
            uartrxbuff.cpp
            variant8.cpp
            version.cpp
            w25x.cpp
            w25x_communication.cpp
            wdt.cpp
            z_calibration_fsm.cpp
            Z_probe.cpp
    )
  if(BOARD MATCHES "XLBUDDY")
    target_sources(firmware PRIVATE hwio_XLBuddy.cpp)
  else()
    target_sources(firmware PRIVATE hwio_buddy_2209_02.cpp)
  endif()
elseif(BOARD MATCHES "DWARF")
  target_sources(
    firmware
    PRIVATE adc.cpp
            heap.cpp
            hx717.cpp
            marlin_log_component.cpp
            otp.cpp
            Pin.cpp
            safe_state.cpp
            timing.cpp
    )
elseif(BOARD MATCHES "MODULARBED")
  target_sources(firmware PRIVATE heap.cpp otp.cpp)
endif()

if(PRINTER STREQUAL "MINI")
  target_sources(firmware PRIVATE minda_broken_cable_detection.cpp MindaRedscreen.cpp)
endif()

if(NOT BOARD STREQUAL "DWARF" AND NOT BOARD STREQUAL "MODULARBED")
  if(PRINTER IN_LIST PRINTERS_WITH_POWER_PANIC)
    target_sources(firmware PRIVATE power_panic.cpp)
  endif()
endif()

if(FILAMENT_SENSOR)
  target_sources(
    firmware PRIVATE filament_sensor.cpp filament_sensors_handler.cpp thread_measurement.cpp
    )
  if(FILAMENT_SENSOR STREQUAL "ADC")
    target_sources(firmware PRIVATE filament_sensor_adc.cpp)
  else()
    target_sources(firmware PRIVATE filament_sensor_photoelectric.cpp)
  endif()
  if(HAS_MMU2)
    target_sources(firmware PRIVATE filament_sensor_mmu.cpp)
  endif()
endif()

if(HAS_LOADCELL)
  target_sources(firmware PRIVATE loadcell.cpp)
endif()

if(NOT BOARD STREQUAL "DWARF" AND NOT BOARD STREQUAL "MODULARBED")
  if(BOARD MATCHES "XLBUDDY")
    target_sources(firmware PRIVATE filament_sensors_handler_toolchanger.cpp)
  elseif(HAS_MMU2)
    target_sources(firmware PRIVATE filament_sensors_handler_mmu.cpp)
  else()
    target_sources(firmware PRIVATE filament_sensors_handler_no_mmu.cpp)
  endif()
endif()

if(GUI)
  # TODO: Some of it should be moved under GUI
  target_sources(firmware PRIVATE bsod_gui.cpp filament.cpp)
endif()

if(NOT BOARD STREQUAL "DWARF" AND NOT BOARD STREQUAL "MODULARBED")
  if(HAS_SELFTEST)
    add_subdirectory(selftest)
  endif()
endif()

if(HAS_ADVANCED_POWER)
  target_sources(firmware PRIVATE advanced_power.cpp)
endif()

add_subdirectory(filters)
add_subdirectory(fanctl)

if(BOARD MATCHES "BUDDY")
  add_subdirectory(crash_dump)
  add_subdirectory(automata)
  add_subdirectory(http)
endif()

add_subdirectory(utils)
