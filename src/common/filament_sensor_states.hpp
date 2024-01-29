/// @file
#pragma once
#include <stdint.h>

enum class FilamentSensorState : uint8_t {
    NotInitialized, // enable enters this state too
    NotCalibrated,
    HasFilament,
    NoFilament,
    NotConnected,
    Disabled
};
