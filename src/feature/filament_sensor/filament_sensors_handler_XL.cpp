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
FSensorADC *getExtruderFSensor(PhysicalToolIndex index) {
    static StrongIndexArray<FSensorADC, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> printer_sensors = { {
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 0 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 1 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 2 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 3 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 4 } },
    } };

    if (index.is_enabled()) {
        return &printer_sensors[index];
    }
    return nullptr;
}

// Meyer's singleton
FSensorADC *getSideFSensor(PhysicalToolIndex index) {
    static StrongIndexArray<FSensorADC, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> side_sensors = { {
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 0 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 1 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 2 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 3 } },
        FSensorADC { FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 4 } },
    } };

    if (index.is_enabled()) {
        return &side_sensors[index];
    }
    return nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(PhysicalToolIndex index) {
    return getExtruderFSensor(index);
}

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor(PhysicalToolIndex index) {
    return getSideFSensor(index);
}

// IRQ - called from interruption
void fs_process_sample(int32_t fs_raw_value, PhysicalToolIndex tool_index) {
    FSensorADC *sensor = getExtruderFSensor(tool_index);
    debug_assert(sensor);

    // does not need to be filtered (data from tool are already filtered)
    sensor->set_filtered_value_from_IRQ(fs_raw_value);
}

void side_fs_process_sample(int32_t fs_raw_value, PhysicalToolIndex tool_index) {
    static StrongIndexArray<MedianFilter, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> filters;

    FSensorADC *sensor = getSideFSensor(tool_index);
    debug_assert(sensor);

    auto &filter = filters[tool_index];

    sensor->set_filtered_value_from_IRQ(filter.filter(fs_raw_value) ? fs_raw_value : FSensorADCEval::filtered_value_not_ready);
}
