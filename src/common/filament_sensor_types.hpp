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
    std::optional<IFSensor::Event> primary_runout = std::nullopt;
    std::optional<IFSensor::Event> secondary_runout = std::nullopt;
    std::optional<IFSensor::Event> autoload = std::nullopt;
    std::optional<IFSensor::Event> current_extruder = std::nullopt;
    std::optional<IFSensor::Event> current_side = std::nullopt;

    std::optional<IFSensor::Event> &get(size_t idx) {
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
            static std::optional<IFSensor::Event> empty = std::nullopt;
            return empty;
        }
        }
    }
};

} // namespace filament_sensor
