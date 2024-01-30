/**
 * @file filament_sensors_handler_no_side_sensor.cpp
 * @brief this file contains dummy side sensor code for printers without it
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_types.hpp"
#include <cassert>

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor([[maybe_unused]] uint8_t index) {
    return nullptr;
}

// printer wit just 1 filament sensor never needs to reconfigure
void FilamentSensors::reconfigure_sensors_if_needed(bool force) {
    if (!force) {
        return;
    }

    tool_index = 0;

    using LFS = LogicalFilamentSensor;
    auto &ls = logical_sensors_;

    const auto extruder_fs = GetExtruderFSensor(tool_index);

    ls[LFS::primary_runout] = extruder_fs;
    ls[LFS::autoload] = extruder_fs;
}

void FilamentSensors::AdcSide_FilteredIRQ([[maybe_unused]] int32_t val, [[maybe_unused]] uint8_t tool_index) {
    bsod("no adc sensor");
}

void side_fs_process_sample([[maybe_unused]] int32_t fs_raw_value, [[maybe_unused]] uint8_t tool_index) {
    bsod("no adc sensor");
}
