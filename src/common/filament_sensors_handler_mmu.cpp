/**
 * @file filament_sensors_handler_mmu.cpp
 * @brief this file contains code MMU for filament sensor api with MMU support
 * in case of MMU, MMU sensor == side sensor
 */

#include "filament_sensors_handler.hpp"
#include "filament_sensor_mmu.hpp"
#include "filament_sensor_adc.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#include "marlin_client.hpp"
#include "metric.h"
#include <common/freertos_mutex.hpp>
#include "filters/median_filter.hpp"
#include <mutex>
#include <config_store/store_instance.hpp>

using namespace MMU2;

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor(uint8_t index) {
    static FSensorMMU side_sensor;
    return index == 0 && config_store().mmu2_enabled.get() ? &side_sensor : nullptr;
}
