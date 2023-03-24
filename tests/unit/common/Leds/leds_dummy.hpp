#pragma once
#include <array>
#include "gui_leds.hpp"

template <size_t COUNT>
struct LedsDummy {
    std::array<leds::Color, COUNT> leds;
    void Reset() {
        leds.fill(leds::Color { 0 });
    }
};

using MK4_leds = LedsDummy<3>;

extern MK4_leds &GetLeds();
