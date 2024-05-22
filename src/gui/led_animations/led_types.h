#pragma once

#include <stdint.h>
#include <tuple>
#include <cmath>
#include "bsod.h"
#include <leds/color.hpp>

enum class PrinterState : uint16_t {
    Idle,
    Printing,
    Pausing,
    Resuming,
    Aborting,
    Finishing,
    Warning,
    PowerPanic,
    PowerUp,
    _count = PowerUp,
};

enum class AnimationTypes : uint8_t {
    SolidColor,
    Fading,
    _count = Fading
};
