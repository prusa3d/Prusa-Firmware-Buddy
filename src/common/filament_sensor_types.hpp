/**
 * @file filament_sensor_types.hpp
 */

#pragma once

#include "stdint.h"
#include "filament_sensor.hpp"
#include <optional>
#include <array>

enum class LogicalFilamentSensor : uint8_t {
    /// Filament sensor on the current extruder
    extruder,
    /// Side sensor for the current extruder
    /// MK4+MMU: MMU sensor | XL: current side sensor | OTHER: none
    side,
    /// The first runout filament sensor - the one further from the extruder
    /// XL: side sensor | MK4+MMU: MMU sensor | OTHER: extruder sensor
    primary_runout,
    /// The second runout filament sensor - the one closer to the extruder
    /// XL,MK4+MMU: extruder sensor | OTHER: none
    secondary_runout,
    /// Filament sensor that is used to detect autoload
    autoload,
};

static constexpr size_t logical_filament_sensor_count = 5;

struct LogicalFilamentSensors {
    std::array<std::atomic<IFSensor *>, logical_filament_sensor_count> array = { nullptr };

    inline auto &operator[](LogicalFilamentSensor fs) {
        return array.at(ftrstd::to_underlying(fs));
    }
    inline IFSensor *operator[](LogicalFilamentSensor fs) const {
        return array.at(ftrstd::to_underlying(fs));
    }
};

// We need those. States obtained from from sensors directly might not by synchronized
struct LogicalFilamentSensorStates {
    using State = std::atomic<FilamentSensorState>;
    static constexpr const FilamentSensorState init_val = FilamentSensorState::NotInitialized;
    std::array<State, logical_filament_sensor_count> array = { init_val };

    inline auto &operator[](LogicalFilamentSensor fs) {
        return array.at(ftrstd::to_underlying(fs));
    }
    inline const auto &operator[](LogicalFilamentSensor fs) const {
        return array.at(ftrstd::to_underlying(fs));
    }
};
