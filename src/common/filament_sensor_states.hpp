/// @file
#pragma once
#include <stdint.h>

enum class fsensor_t : uint8_t {
    NotInitialized, // enable enters this state too
    NotCalibrated,
    HasFilament,
    NoFilament,
    NotConnected,
    Disabled
};
