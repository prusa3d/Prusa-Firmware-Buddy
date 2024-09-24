target_include_directories(firmware PRIVATE . include)

if(BOARD MATCHES ".*BUDDY")
  target_sources(
    firmware
    PRIVATE adc.cpp
            app_metrics.cpp
            appmain.cpp
            base64_stream_decoder.cpp
            bbf.cpp
            crc32.cpp
            crc32_zlib.cpp
            data_exchange.cpp
            enum_array.cpp
            error_code_mangle.cpp
            ExtInterruptHandler.cpp
            filename_type.cpp
            filepath_operation.cpp
            footer_eeprom.cpp
            fsm_network_setup.cpp
            fsm_states.cpp
            general_response.cpp
            hardware_serial.cpp
            heap.cpp
            homing_reporter.cpp
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
            PersistentStorage.cpp
            Pin.cpp
            ping_manager.cpp
            print_utils.cpp
            printer_model.cpp
            probe_analysis.cpp
            random_hw.cpp
            rw_mutex.cpp
            safe_state.cpp
            segmented_json.cpp
            selftest_result.cpp
            selftest_result_type.cpp
            sensor_data.cpp
            serial_printing.cpp
            sound.cpp
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
            tools_mapping.cpp
            trinamic.cpp
            w25x.cpp
            w25x_communication.cpp
            wdt.cpp
            z_calibration_fsm.cpp
    )
  add_subdirectory(async_job)

  if(BOARD MATCHES "XLBUDDY")
    target_sources(firmware PRIVATE hwio_XLBuddy.cpp)
  else()
    target_sources(firmware PRIVATE hwio_buddy_2209_02.cpp)
  endif()

  if(HAS_SHEET_PROFILES)
    target_sources(firmware PRIVATE SteelSheets.cpp)
  endif()
elseif(BOARD MATCHES "DWARF")
  target_sources(
    firmware
    PRIVATE adc.cpp
            heap.cpp
            homing_reporter.cpp
            marlin_log_component.cpp
            otp.cpp
            Pin.cpp
            random_sw.cpp
            safe_state.cpp
    )
  target_link_libraries(firmware PRIVATE logging)
elseif(BOARD MATCHES "MODULARBED")
  target_sources(firmware PRIVATE heap.cpp otp.cpp)
  target_link_libraries(firmware PRIVATE logging)
elseif(BOARD MATCHES "XL_DEV_KIT_XLB")
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
            homing_reporter.cpp
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
            serial_printing.cpp
            sound.cpp
            st25dv64k.cpp # todo: remove
            tasks.cpp
            tools_mapping.cpp
            trinamic.cpp
            wdt.cpp
            z_calibration_fsm.cpp
    )
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
    target_sources(firmware PRIVATE filament_sensor_adc.cpp filament_sensor_adc_eval.cpp)
  else()
    target_sources(firmware PRIVATE filament_sensor_photoelectric.cpp)
  endif()
  if(HAS_MMU2)
    target_sources(firmware PRIVATE filament_sensor_mmu.cpp)
  endif()
  if(PRINTER STREQUAL "iX")
    target_sources(firmware PRIVATE filament_sensor_ix_side.cpp)
  endif()
endif()

if(HAS_LOADCELL)
  target_sources(firmware PRIVATE loadcell.cpp probe_position_lookback.cpp)
endif()
if(HAS_LOADCELL_HX717)
  target_sources(firmware PRIVATE hx717.cpp)
endif()
if(HAS_LOADCELL AND HAS_LOADCELL_HX717)
  target_sources(firmware PRIVATE hx717mux.cpp)
endif()

if((BOARD STREQUAL "XBUDDY") OR (BOARD STREQUAL "BUDDY"))
  if(HAS_MMU2)
    target_sources(firmware PRIVATE filament_sensors_handler_mmu.cpp)
  elseif(PRINTER STREQUAL "iX")
    # iX has a side sensor, set up in filament_sensors_handler_iX.cpp
  else()
    target_sources(firmware PRIVATE filament_sensors_handler_no_side_sensor.cpp)
  endif()
endif()

if(NOT BOARD STREQUAL "DWARF" AND NOT BOARD STREQUAL "MODULARBED")
  if(PRINTER STREQUAL "XL")
    target_sources(firmware PRIVATE filament_sensors_handler_XL_remap.cpp)
  endif()

  target_sources(firmware PRIVATE filament_sensors_handler_${PRINTER}.cpp)
endif()

if(GUI)
  # TODO: Some of it should be moved under GUI
  target_sources(
    firmware PRIVATE bsod_gui.cpp filament.cpp filament_gui.cpp filament_list.cpp
                     filament_to_load.cpp
    )
endif()

if(NOT BOARD STREQUAL "DWARF" AND NOT BOARD STREQUAL "MODULARBED")
  if(HAS_SELFTEST)
    add_subdirectory(selftest)
  endif()
endif()

if(HAS_ADVANCED_POWER)
  target_sources(firmware PRIVATE advanced_power.cpp)
endif()

if(HAS_NFC)
  target_sources(firmware PRIVATE nfc.cpp)
endif()

add_subdirectory(filters)
add_subdirectory(fanctl)
add_subdirectory(mapi)
add_subdirectory(marlin_server_types)

if(BOARD MATCHES "BUDDY")
  add_subdirectory(crash_dump)
  add_subdirectory(automata)
  add_subdirectory(http)
endif()
if(BOARD_IS_MASTER_BOARD)
  add_subdirectory(gcode)
  add_subdirectory(feature)
endif()

add_subdirectory(utils)
