/**
 * @file filament_sensors_handler_iX.cpp
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_types.hpp"
#include "filament_sensor_adc.hpp"
#include "filament_sensor_adc_eval.hpp"
#include "metric.h"
#include "filters/median_filter.hpp"

// Meyer's singleton
static FSensorAdcExtruder *getExtruderFSensor(uint8_t index) {
    static FSensorAdcExtruder printer_sensor = FSensorAdcExtruder(0, false);

    return index == 0 ? &printer_sensor : nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(uint8_t index) {
    return getExtruderFSensor(index);
}

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

    if (filter.filter(fs_raw_value)) { // fs_raw_value is rewritten - passed by reference
        FSensors_instance().AdcExtruder_FilteredIRQ(fs_raw_value, tool_index);
    } else {
        FSensors_instance().AdcExtruder_FilteredIRQ(FSensorADCEval::filtered_value_not_ready, tool_index);
    }
}
