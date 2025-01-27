/**
 * @file filament_sensors_handler_mmu.cpp
 * @brief this file contains code MMU for filament sensor api with MMU support
 * in case of MMU, MMU sensor == side sensor
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_mmu.hpp"
#include <common/freertos_mutex.hpp>
#include <config_store/store_instance.hpp>

using namespace MMU2;

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor(uint8_t index) {
    static FSensorMMU side_sensor;
    return index == 0 && config_store().mmu2_enabled.get() ? &side_sensor : nullptr;
}
