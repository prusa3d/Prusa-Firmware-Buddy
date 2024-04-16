/// @file
#pragma once
#include <stdint.h>

enum class FilamentSensorState : uint8_t {
    NotInitialized, // enable enters this state too
    NotCalibrated,
    HasFilament,
    NoFilament,
    NotConnected,
    Disabled,
};

/// Returns whether the passed FilamentSensorState means that the filament sensor is working (functional, set up and such)
constexpr inline bool is_fsensor_working_state(FilamentSensorState state) {
    return state == FilamentSensorState::NoFilament || state == FilamentSensorState::HasFilament;
}
