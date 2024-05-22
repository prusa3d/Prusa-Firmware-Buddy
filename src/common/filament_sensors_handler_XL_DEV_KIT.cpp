/**
 * @file filament_sensors_handler_XL.cpp
 * @brief this file contains code for filament sensor api with multi tool support
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_adc.hpp"
#include "filament_sensor_adc_eval.hpp"
#include "filters/median_filter.hpp"
#include "marlin_client.hpp"
#include <common/freertos_mutex.hpp>
#include <mutex>
#include <puppies/Dwarf.hpp>
#include "src/module/prusa/toolchanger.h"

// Meyer's singleton
FSensorADC *getExtruderFSensor(uint8_t index) {
    static std::array<FSensorADC, EXTRUDERS> printer_sensors = { {
        { 0, false },
        { 1, false },
        { 2, false },
        { 3, false },
        { 4, false },
        { 5, false },
    } };

    return (index < 5 && prusa_toolchanger.is_tool_enabled(index)) ? &printer_sensors[index] : nullptr; // 6th sensor is not calibrated and causing errors
}

// Meyer's singleton
FSensorADC *getSideFSensor(uint8_t index) {
    static std::array<FSensorADC, EXTRUDERS> side_sensors = { {
        { 0, true },
        { 1, true },
        { 2, true },
        { 3, true },
        { 4, true },
        { 5, true },
    } };
    return (index < 5 && prusa_toolchanger.is_tool_enabled(index)) ? &side_sensors[index] : nullptr; // 6th sensor is not calibrated and causing errors
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(uint8_t index) {
    return getExtruderFSensor(index);
}

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor(uint8_t index) {
    return getSideFSensor(index);
}

// IRQ - called from interruption
void fs_process_sample(int32_t fs_raw_value, uint8_t tool_index) {
    FSensorADC *sensor = getExtruderFSensor(tool_index);
    assert(sensor);

    // does not need to be filtered (data from tool are already filtered)
    sensor->set_filtered_value_from_IRQ(fs_raw_value);
}

void side_fs_process_sample(int32_t fs_raw_value, uint8_t tool_index) {
    static MedianFilter filters[HOTENDS];

    FSensorADC *sensor = getSideFSensor(tool_index);
    assert(sensor);

    auto &filter = filters[tool_index];

    sensor->record_raw(fs_raw_value);
    sensor->set_filtered_value_from_IRQ(filter.filter(fs_raw_value) ? fs_raw_value : FSensorADCEval::filtered_value_not_ready);
}
