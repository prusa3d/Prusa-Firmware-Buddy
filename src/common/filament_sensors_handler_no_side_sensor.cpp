/**
 * @file filament_sensors_handler_no_side_sensor.cpp
 * @brief this file contains dummy side sensor code for printers without it
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_types.hpp"
#include <cassert>

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor([[maybe_unused]] uint8_t index) {
    return nullptr;
}
