/**
 * @file filament_sensors_handler_no_side_sensor.cpp
 * @brief this file contains dummy side sensor code for printers without it
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_types.hpp"
#include <cassert>

bool FilamentSensors::has_mmu2_enabled() const {
    return false;
}

void FilamentSensors::process_side_request() {
}

void FilamentSensors::DisableSideSensor() {
}

filament_sensor::mmu_enable_result_t FilamentSensors::EnableSide() {
    return filament_sensor::mmu_enable_result_t::error_mmu_not_supported;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor([[maybe_unused]] uint8_t index) {
    return nullptr;
}

void FilamentSensors::SetToolIndex() {
    tool_index = 0;
}

// printer wit just 1 filament sensor never needs to reconfigure
void FilamentSensors::reconfigure_sensors_if_needed() {
}

void FilamentSensors::configure_sensors() {
    logical_sensors.current_extruder = GetExtruderFSensor(tool_index);

    logical_sensors.primary_runout = logical_sensors.current_extruder;
    logical_sensors.secondary_runout = nullptr;
    logical_sensors.autoload = logical_sensors.current_extruder;
}
void FilamentSensors::AdcSide_FilteredIRQ([[maybe_unused]] int32_t val, [[maybe_unused]] uint8_t tool_index) {
    assert("no adc sensor");
}

void side_fs_process_sample([[maybe_unused]] int32_t fs_raw_value, [[maybe_unused]] uint8_t tool_index) {
    assert("no adc sensor");
}
