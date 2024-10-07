#pragma once
#include "color.hpp"

namespace leds {

class LedStrip {
public:
    virtual int GetLedCount() = 0;
    virtual ColorRGBW GetColor(int led_idx) = 0;
    virtual void SetColor(int led_idx, ColorRGBW color) = 0;
    virtual void SetColor(ColorRGBW color) = 0;
};

} // namespace leds
