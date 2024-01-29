/**
 * @file filament_sensors_handler_mmu.cpp
 * @brief this file contains code MMU for filament sensor api with MMU support
 * in case of MMU, MMU sensor == side sensor
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_mmu.hpp"
#include "filament_sensor_adc.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#include "marlin_client.hpp"
#include "metric.h"
#include <common/freertos_mutex.hpp>
#include "filters/median_filter.hpp"
#include <mutex>
#include <config_store/store_instance.hpp>

using namespace MMU2;

// Meyer's singleton
static FSensorMMU *getSideFSensor(uint8_t index) {
    static FSensorMMU side_sensor;
    return index == 0 && config_store().mmu2_enabled.get() ? &side_sensor : nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor(uint8_t index) {
    return getSideFSensor(index);
}

void FilamentSensors::SetToolIndex() {
    tool_index = 0;
}

void FilamentSensors::configure_sensors() {
    auto side_sensor_state = mmu2.State();
    has_mmu = !(side_sensor_state == xState::Stopped);

    logical_sensors.current_extruder = GetExtruderFSensor(tool_index);
    logical_sensors.current_side = GetSideFSensor(tool_index);

    logical_sensors.primary_runout = has_mmu ? logical_sensors.current_side : logical_sensors.current_extruder;
    logical_sensors.secondary_runout = has_mmu ? logical_sensors.current_extruder : nullptr;
    logical_sensors.autoload = has_mmu ? nullptr : logical_sensors.current_extruder;
}

void FilamentSensors::reconfigure_sensors_if_needed() {
    // auto static old_mmu_state = mmu2.State();
    // auto mmu_state = mmu2.State();
    // if (mmu_state != old_mmu_state) {
    configure_sensors();
    //    old_mmu_state = mmu_state;
    //}
}

void FilamentSensors::AdcSide_FilteredIRQ([[maybe_unused]] int32_t val, [[maybe_unused]] uint8_t tool_index) {
    bsod("no adc sensor");
}

void side_fs_process_sample([[maybe_unused]] int32_t fs_raw_value, [[maybe_unused]] uint8_t tool_index) {
}
