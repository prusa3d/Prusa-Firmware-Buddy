/**
 * @file filament_sensors_handler_no_mmu.cpp
 * @brief this file contains code MMU for filament sensor api without MMU support
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_types.hpp"
#include <printers.h>

#if PRINTER_TYPE == PRINTER_PRUSA_MINI
    #include "filament_sensor_photoelectric.hpp"
#else
    #include "filament_sensor_adc.hpp"
    #include "metric.h"
    #include "filters/median_filter.hpp"
#endif

bool FilamentSensors::has_mmu2_enabled() {
    return false;
}

void FilamentSensors::process_side_request() {
}

void FilamentSensors::DisableSideSensor() {
}

filament_sensor::mmu_enable_result_t FilamentSensors::EnableSide() {
    return filament_sensor::mmu_enable_result_t::error_mmu_not_supported;
}

#if PRINTER_TYPE == PRINTER_PRUSA_MINI
// Meyer's singleton
static FSensorPhotoElectric *getExtruderFSensor(uint8_t index) {
    static FSensorPhotoElectric printer_sensor = {};

    return index == 0 ? &printer_sensor : nullptr;
}
#else
// Meyer's singleton
static FSensorAdcExtruder *getExtruderFSensor(uint8_t index) {
    static FSensorAdcExtruder printer_sensor = FSensorAdcExtruder(EEVAR_FS_VALUE_SPAN_0, EEVAR_FS_REF_VALUE_0, 0);

    return index == 0 ? &printer_sensor : nullptr;
}
#endif

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(uint8_t index) {
    return getExtruderFSensor(index);
}

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor([[maybe_unused]] uint8_t index) {
    return nullptr;
}

void FilamentSensors::SetToolIndex() {
    tool_index = 0;
}

// MINI never needs to reconfigure
void FilamentSensors::reconfigure_sensors_if_needed() {
}

void FilamentSensors::configure_sensors() {
    logical_sensors.primary_runout = GetExtruderFSensor(tool_index);
    logical_sensors.secondary_runout = nullptr;
    logical_sensors.autoload = GetExtruderFSensor(tool_index);

    physical_sensors.current_extruder = GetExtruderFSensor(tool_index);
}
void FilamentSensors::AdcSide_FilteredIRQ([[maybe_unused]] int32_t val, [[maybe_unused]] uint8_t tool_index) {
    // might want to log error .. no adc sensor
}

void side_fs_process_sample([[maybe_unused]] int32_t fs_raw_value, [[maybe_unused]] uint8_t tool_index) {
    // might want to log error .. no adc sensor
}

#if PRINTER_TYPE == PRINTER_PRUSA_MINI

void FilamentSensors::AdcExtruder_FilteredIRQ([[maybe_unused]] int32_t val, [[maybe_unused]] uint8_t tool_index) {
    // might want to log error .. no adc sensor
}

// IRQ - called from interruption
void fs_process_sample([[maybe_unused]] int32_t fs_raw_value, [[maybe_unused]] uint8_t tool_index) {
    // might want to log error .. no adc sensor
}

#else // iX on some other printer with ADC sensor, but without MMU

void FilamentSensors::AdcExtruder_FilteredIRQ(int32_t val, uint8_t tool_index) {
    FSensorADC *sensor = getExtruderFSensor(tool_index);
    if (sensor) {
        sensor->set_filtered_value_from_IRQ(val);
    } else {
        assert("wrong extruder index");
    }
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

#endif
