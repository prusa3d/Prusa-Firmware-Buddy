/**
 * @file filament_sensor_types.hpp
 */

#pragma once

#include "stdint.h"
#include "filament_sensor.hpp"
#include <optional>
#include <array>

namespace filament_sensor {

struct LogicalSensors {
    IFSensor *primary_runout = nullptr; // XL side sensor; printer with MMU : MMU; printer without MMU extruder sensor
    IFSensor *secondary_runout = nullptr; // XL, printer with MMU, extruder sensor; other don't have
    IFSensor *autoload = nullptr; // printer with MMU dont have, XL side
    IFSensor *current_extruder = nullptr; // sensor on extruder, on XL it is bound to current extruder
    IFSensor *current_side = nullptr; // on MK4 this is MMU sensor, on XL it is currently selected side sensors, MINI does not have one
    // combination XL + MMU currently unsupported

    auto get_array() {
        return std::to_array<IFSensor *>({ primary_runout, secondary_runout, autoload, current_extruder, current_side });
    }
};

struct Events {
    std::optional<IFSensor::event> primary_runout = std::nullopt;
    std::optional<IFSensor::event> secondary_runout = std::nullopt;
    std::optional<IFSensor::event> autoload = std::nullopt;
    std::optional<IFSensor::event> current_extruder = std::nullopt;
    std::optional<IFSensor::event> current_side = std::nullopt;

    std::optional<IFSensor::event> &get(size_t idx) {
        switch (idx) {
        case 0:
            return primary_runout;
        case 1:
            return secondary_runout;
        case 2:
            return autoload;
        case 3:
            return current_extruder;
        case 4:
            return current_side;
        default: {
            static std::optional<IFSensor::event> empty = std::nullopt;
            return empty;
        }
        }
    }
};

/**
 * @brief what mas happen to inject M600
 */
enum class inject_t : int32_t {
    on_edge = 0,
    on_level = 1,
    never = 2
};

/**
 * @brief command
 * you cannot change state of sensor directly
 * it would not be thread safe
 */
enum class cmd_t : int32_t {
    null, // no command
    on, // request to turn on
    off, // request to turn off
    processing, // currently processing command
};

/**
 * @brief shared state of extruder + side sensor
 * so it can be atomically accessed
 * ordered by evaluation priority
 */
enum class init_status_t : int32_t {
    NotReady, // cannot evaluate right now, commands being processed
    BothNotCalibrated,
    SideNotCalibrated,
    ExtruderNotCalibrated,
    BothNotInitialized,
    SideNotInitialized,
    ExtruderNotInitialized,
    Ok, // neither of other states :)
};

enum class mmu_enable_result_t : int32_t {
    ok,
    error_filament_sensor_disabled,
    error_mmu_not_supported
};
} // namespace filament_sensor
