/**
 * @file filament_sensors_handler_iX.cpp
 */

#include "filament_sensor_ix_side.hpp"
#include "filament_sensors_handler.hpp"
#include "filament_sensor_types.hpp"
#include "filament_sensor_adc.hpp"
#include "filament_sensor_adc_eval.hpp"
#include "metric.h"
#include "filters/median_filter.hpp"

// Meyer's singleton
static FSensorADC *getExtruderFSensor(uint8_t index) {
    static FSensorADC printer_sensor = FSensorADC(0, false);
    return (index == 0) ? &printer_sensor : nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(uint8_t index) {
    return getExtruderFSensor(index);
}

IFSensor *GetSideFSensor(uint8_t index) {
    static FSensor_iXSide sensor;
    return (index == 0) ? &sensor : nullptr;
}

// IRQ - called from interruption
void fs_process_sample(int32_t fs_raw_value, uint8_t tool_index) {
    static MedianFilter filter;

    FSensorADC *sensor = getExtruderFSensor(tool_index);
    assert(sensor);

    sensor->record_raw(fs_raw_value);
    sensor->set_filtered_value_from_IRQ(filter.filter(fs_raw_value) ? fs_raw_value : FSensorADCEval::filtered_value_not_ready);
}
