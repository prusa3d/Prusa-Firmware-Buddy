/**
 * @file filament_sensors_handler_mmu.cpp
 * @brief this file contains code MMU for filament sensor api with MMU support
 * in case of MMU, MMU sensor == side sensor
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_mmu.hpp"
#include "filament_sensor_adc.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2mk4.h"
#include "eeprom.h"
#include "marlin_client.hpp"
#include "metric.h"
#include "freertos_mutex.hpp"
#include "filters/median_filter.hpp"
#include <mutex>

using namespace MMU2;

bool FilamentSensors::has_mmu2_enabled() {
    return eeprom_get_bool(EEVAR_MMU2_ENABLED);
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
    if ((!physical_sensors.current_extruder) || (!IsWorking(physical_sensors.current_extruder->Get()))) { // physical_sensors.current_extruder is not synchronized, but in this case it it OK
        return filament_sensor::mmu_enable_result_t::error_filament_sensor_disabled;
    }
    const std::lock_guard lock(GetSideMutex());
    request_side = filament_sensor::cmd_t::on;
    return filament_sensor::mmu_enable_result_t::ok;
}

static void mmu_disable() {
    marlin_gcode("M709 S0");
    eeprom_set_bool(EEVAR_MMU2_ENABLED, false);
}

// cannot wait until it is enabled, it takes like 10 seconds !!!
static void mmu_enable() {
    marlin_gcode("M709 S1");
    eeprom_set_bool(EEVAR_MMU2_ENABLED, true);
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
static FSensorAdcExtruder *getExtruderFSensor(uint8_t index) {
    static FSensorAdcExtruder printer_sensor = FSensorAdcExtruder(EEVAR_FS_VALUE_SPAN_0, EEVAR_FS_REF_VALUE_0, 0);

    return index == 0 ? &printer_sensor : nullptr;
}

// Meyer's singleton
static FSensorMMU *getSideFSensor(uint8_t index) {
    static FSensorMMU side_sensor;
    return index == 0 && eeprom_get_bool(EEVAR_MMU2_ENABLED) ? &side_sensor : nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(uint8_t index) {
    return getExtruderFSensor(index);
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
    has_mmu = !(side_sensor_state == State_t::Stopped);

    physical_sensors.current_extruder = GetExtruderFSensor(tool_index);
    physical_sensors.current_side = GetSideFSensor(tool_index);

    logical_sensors.primary_runout = has_mmu ? GetSideFSensor(tool_index) : GetExtruderFSensor(tool_index);
    logical_sensors.secondary_runout = has_mmu ? GetExtruderFSensor(tool_index) : nullptr;
    logical_sensors.autoload = has_mmu ? nullptr : GetExtruderFSensor(tool_index);
}

void FilamentSensors::reconfigure_sensors_if_needed() {
    auto static old_mmu_state = mmu2.State();
    auto mmu_state = mmu2.State();
    if (mmu_state != old_mmu_state) {
        configure_sensors();
        old_mmu_state = mmu_state;
    }
}

void FilamentSensors::AdcExtruder_FilteredIRQ(int32_t val, uint8_t tool_index) {
    FSensorADC *sensor = getExtruderFSensor(tool_index);
    if (sensor) {
        sensor->set_filtered_value_from_IRQ(val);
    } else {
        assert("wrong extruder index");
    }
}

void FilamentSensors::AdcSide_FilteredIRQ(int32_t val, uint8_t tool_index) {
    assert("no adc sensor");
}

// IRQ - called from interruption
void fs_process_sample(int32_t fs_raw_value, uint8_t tool_index) {
    static MedianFilter filter;

    FSensorADC *sensor = getExtruderFSensor(tool_index);
    if (sensor) {
        sensor->record_raw(fs_raw_value);
    }

    if (filter.filter(fs_raw_value)) { //fs_raw_value is rewritten - passed by reference
        FSensors_instance().AdcExtruder_FilteredIRQ(fs_raw_value, tool_index);
    } else {
        FSensors_instance().AdcExtruder_FilteredIRQ(FSensorADC::fs_filtered_value_not_ready, tool_index);
    }
}

void side_fs_process_sample(int32_t fs_raw_value, uint8_t tool_index) {
}
