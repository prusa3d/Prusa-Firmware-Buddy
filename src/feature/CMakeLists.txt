if(HAS_GEARBOX_ALIGNMENT)
  add_subdirectory(gearbox_alignment)
endif()

if(HAS_NOZZLE_CLEANER)
  add_subdirectory(nozzle_cleaner)
endif()

if(HAS_NOZZLE_CLEANER_LITE)
  add_subdirectory(nozzle_cleaner_lite)
endif()

if(HAS_WASTEBIN_FILL_TRACKING)
  add_subdirectory(wastebin_watcher)
endif()

if(HAS_DOOR_SENSOR_CALIBRATION)
  add_subdirectory(door_sensor_calibration)
endif()

if(HAS_TOOL_OFFSET_SENSOR)
  target_sources(firmware PRIVATE contactless_offset/can_remote_sensor_glue.cpp)
  target_link_libraries(firmware PUBLIC contactless_offset)
  add_subdirectory(tool_offset_calibration)
  add_subdirectory(tool_offset_wizard)
endif()

if(HAS_INDX)
  add_subdirectory(indx_dock_calibration)
  add_subdirectory(indx_nozzle_cleaner_calibration)
  add_subdirectory(indx_hotend_temp_model)
  add_subdirectory(indx_tool_lock_hack)
  add_subdirectory(indx_head_leds)
endif()

if(HAS_CANCEL_OBJECT)
  add_subdirectory(cancel_object)
endif()

if(HAS_MANUAL_BELT_TUNING)
  add_subdirectory(manual_belt_tuning)
endif()

if(HAS_XBUDDY_EXTENSION)
  add_subdirectory(xbuddy_extension)
endif()

if(HAS_CHAMBER_API)
  add_subdirectory(chamber)
endif()

if(HAS_CHAMBER_FILTRATION_API)
  add_subdirectory(chamber_filtration)
endif()

if(HAS_EMERGENCY_STOP)
  add_subdirectory(emergency_stop)
endif()

if(HAS_HT_HOTEND)
  target_sources(firmware PRIVATE hotend_burn_risk.cpp)
endif()

if(HAS_CEILING_CLEARANCE)
  add_subdirectory(ceiling_clearance)
endif()

if(HAS_NOZZLE_THERMAL_COMPENSATION)
  add_subdirectory(nozzle_thermal_compensation)
endif()

add_subdirectory(print_status_message)

if(HAS_AUTO_RETRACT)
  add_subdirectory(auto_retract)
endif()

if(HAS_REMOTE_BED)
  add_subdirectory(remote_bed)
endif()

# Build the filament sensor subsystem if there is any filament sensor
if(NOT FILAMENT_SENSOR STREQUAL "NO" OR HAS_SIDE_FSENSOR)
  add_subdirectory(filament_sensor)
endif()

if(HAS_LOADCELL)
  add_subdirectory(nozzle_cleaning_failed)
endif()

if(HAS_BED_FAN)
  add_subdirectory(bed_fan)
endif()

if(HAS_PSU_FAN)
  add_subdirectory(psu_fan)
endif()

if(HAS_ANFC)
  add_subdirectory(openprinttag)
endif()

if(HAS_FILAMENT_TRACKER)
  add_subdirectory(filament_tracker)
endif()

add_subdirectory(compatibility_checks)
add_subdirectory(factory_reset)
add_subdirectory(motor_current_profile)
add_subdirectory(ramming)
add_subdirectory(cork)
add_subdirectory(safety_timer)
add_subdirectory(stepper_timeout)
add_subdirectory(gcode_exception)

if(RTT_METRICS_ENABLED)
  add_subdirectory(rtt_metrics_task)
endif()
