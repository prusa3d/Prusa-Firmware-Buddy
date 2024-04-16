/**
 * @file filament_sensors_handler_MK4.cpp
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_adc.hpp"
#include "filament_sensor_adc_eval.hpp"
#include "marlin_client.hpp"
#include "metric.h"
#include "filters/median_filter.hpp"

static FSensorADC *getExtruderFSensor(uint8_t index) {
    static FSensorADC printer_sensor(0, false);
    return index == 0 ? &printer_sensor : nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(uint8_t index) {
    return getExtruderFSensor(index);
}

// IRQ - called from interruption
void fs_process_sample(int32_t fs_raw_value, uint8_t tool_index) {
    static MedianFilter filter;

    FSensorADC *sensor = getExtruderFSensor(tool_index);
    assert(sensor);

    sensor->record_raw(fs_raw_value);
    sensor->set_filtered_value_from_IRQ(filter.filter(fs_raw_value) ? fs_raw_value : FSensorADCEval::filtered_value_not_ready);
}
