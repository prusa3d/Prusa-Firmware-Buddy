/**
 * @file filament_sensors_handler_mk3.5.cpp
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_photoelectric.hpp"
#include "marlin_client.hpp"
#include "freertos_mutex.hpp"
#include <mutex>

// Meyer's singleton
static FSensorPhotoElectric *getExtruderFSensor(uint8_t index) {
    static FSensorPhotoElectric printer_sensor = FSensorPhotoElectric();

    return index == 0 ? &printer_sensor : nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(uint8_t index) {
    return getExtruderFSensor(index);
}

void FilamentSensors::AdcExtruder_FilteredIRQ([[maybe_unused]] int32_t val, [[maybe_unused]] uint8_t tool_index) {
    assert("no adc sensor");
}

// IRQ - called from interruption
void fs_process_sample([[maybe_unused]] int32_t fs_raw_value, [[maybe_unused]] uint8_t tool_index) {
    assert("no adc sensor");
}
