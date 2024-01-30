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

// Meyer's singleton
static FSensorMMU *getSideFSensor(uint8_t index) {
    static FSensorMMU side_sensor;
    return index == 0 && config_store().mmu2_enabled.get() ? &side_sensor : nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetSideFSensor(uint8_t index) {
    return getSideFSensor(index);
}

void FilamentSensors::reconfigure_sensors_if_needed([[maybe_unused]] bool force) {
    tool_index = 0;

    auto side_sensor_state = mmu2.State();
    has_mmu = !(side_sensor_state == xState::Stopped);

    using LFS = LogicalFilamentSensor;
    auto &ls = logical_sensors_;

    const auto extruder_fs = GetExtruderFSensor(tool_index);
    const auto side_fs = GetSideFSensor(tool_index);

    ls[LFS::current_extruder] = extruder_fs;
    ls[LFS::current_side] = side_fs;
    ls[LFS::primary_runout] = has_mmu ? side_fs : extruder_fs;
    ls[LFS::secondary_runout] = has_mmu ? extruder_fs : nullptr;
    ls[LFS::autoload] = has_mmu ? nullptr : extruder_fs;
}

void FilamentSensors::AdcSide_FilteredIRQ([[maybe_unused]] int32_t val, [[maybe_unused]] uint8_t tool_index) {
    bsod("no adc sensor");
}

void side_fs_process_sample([[maybe_unused]] int32_t fs_raw_value, [[maybe_unused]] uint8_t tool_index) {
}
