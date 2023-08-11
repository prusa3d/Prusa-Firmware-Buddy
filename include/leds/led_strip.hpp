#pragma once
#include "color.hpp"

namespace leds {

class LedStrip {
public:
    virtual ~LedStrip() = default;
    virtual int GetLedCount() = 0;
    virtual Color GetColor(int led_idx) = 0;
    virtual void SetColor(int led_idx, Color color) = 0;
    virtual void SetColor(Color color) = 0;
};

} // namespace leds
