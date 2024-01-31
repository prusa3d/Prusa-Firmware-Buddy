/**
 * @file filament_sensors_handler_mk3.5.cpp
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_photoelectric.hpp"
#include "marlin_client.hpp"
#include <common/freertos_mutex.hpp>
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
