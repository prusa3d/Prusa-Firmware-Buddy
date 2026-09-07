/**
 * @file filament_sensors_handler_XL.cpp
 * @brief this file contains code for filament sensor api with multi tool support
 */

#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include "filament_sensor_adc.hpp"
#include "filament_sensor_adc_eval.hpp"
#include "filters/median_filter.hpp"
#include "marlin_client.hpp"
#include <freertos/mutex.hpp>
#include <mutex>
#include <puppies/Dwarf.hpp>
#include "src/module/prusa/toolchanger.h"
#include <bsod/bsod.h>

// Meyer's singleton
FSensorADC *getExtruderFSensor(uint8_t index) {
    static StrongIndexArray<FSensorADC, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> printer_sensors = { {
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 0 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 1 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 2 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 3 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 4 } },
    } };

    const auto tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::from_raw_notool(index));
    if (tool.has_value() && tool->is_enabled()) {
        return &printer_sensors[*tool];
    }
    return nullptr;
}

// Meyer's singleton
FSensorADC *getSideFSensor(uint8_t index) {
    static StrongIndexArray<FSensorADC, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> side_sensors = { {
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 0 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 1 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 2 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 3 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 4 } },
    } };

    const auto tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::from_raw_notool(index));
    if (tool.has_value() && tool->is_enabled()) {
        return &side_sensors[*tool];
    }
    return nullptr;
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
    debug_assert(sensor);

    // does not need to be filtered (data from tool are already filtered)
    sensor->set_filtered_value_from_IRQ(fs_raw_value);
}

void side_fs_process_sample(int32_t fs_raw_value, uint8_t tool_index) {
    static MedianFilter filters[HOTENDS];

    FSensorADC *sensor = getSideFSensor(tool_index);
    debug_assert(sensor);

    auto &filter = filters[tool_index];

    sensor->set_filtered_value_from_IRQ(filter.filter(fs_raw_value) ? fs_raw_value : FSensorADCEval::filtered_value_not_ready);
}
