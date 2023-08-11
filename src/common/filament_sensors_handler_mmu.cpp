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
#include "freertos_mutex.hpp"
#include "filters/median_filter.hpp"
#include <mutex>
#include <config_store/store_instance.hpp>

using namespace MMU2;

bool FilamentSensors::has_mmu2_enabled() const {
    return config_store().mmu2_enabled.get();
}

// Store request_side off
void FilamentSensors::DisableSideSensor() {
    const std::lock_guard lock(GetSideMutex());
    request_side = filament_sensor::cmd_t::off;
}

/**
 * @brief store request to enable side sensor
 * there is chance request could not be done
 * there is currently no callback to notify success/failure
 * we could just wait, since request is handled in different thread .. TODO
 * @return filament_sensor::mmu_enable_result_t
 */
filament_sensor::mmu_enable_result_t FilamentSensors::EnableSide() {
    if ((!logical_sensors.current_extruder) || (!IsWorking(logical_sensors.current_extruder->Get()))) { // logical_sensors.current_extruder is not synchronized, but in this case it it OK
        return filament_sensor::mmu_enable_result_t::error_filament_sensor_disabled;
    }
    const std::lock_guard lock(GetSideMutex());
    request_side = filament_sensor::cmd_t::on;
    return filament_sensor::mmu_enable_result_t::ok;
}

static void mmu_disable() {
    marlin_client::gcode("M709 S0");
    config_store().mmu2_enabled.set(false);
}

// cannot wait until it is enabled, it takes like 10 seconds !!!
static void mmu_enable() {
    marlin_client::gcode("M709 S1");
    config_store().mmu2_enabled.set(true);
}

// process side request stored by EnableMMU/DisableSideSensor
void FilamentSensors::process_side_request() {
    switch (request_side) {
    case filament_sensor::cmd_t::on:
        // TODO printer sensor should be working
        // if (IsWorking(state_of_printer_sensor))
        mmu_enable();
        request_side = filament_sensor::cmd_t::processing;
        break;
    case filament_sensor::cmd_t::off:
        mmu_disable();
        request_side = filament_sensor::cmd_t::processing;
        break;
    case filament_sensor::cmd_t::processing:
    case filament_sensor::cmd_t::null:
        break;
    }
}

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
    assert("no adc sensor");
}

void side_fs_process_sample([[maybe_unused]] int32_t fs_raw_value, [[maybe_unused]] uint8_t tool_index) {
}
