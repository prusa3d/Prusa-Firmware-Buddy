#pragma once
#include "led_strip.hpp"
#include <assert.h>
#include <hw/neopixel.hpp>
#include "led_lcd_cs_selector.hpp"
#include "printers.h"
#include <device/board.h>

namespace leds {

class SideStrip : public LedStrip {
public:
    static constexpr bool HasWhiteLed() {
#if PRINTER_IS_PRUSA_XL()
        return true;
#elif PRINTER_IS_PRUSA_iX()
        return false;
#else
    #error "Not defined for this printer."
#endif
    }

    SideStrip()
        : current_color()
        , enclosure_fan_pwm(0)
        , needs_update(true)
        , leds() {
    }

    int GetLedCount() {
        return 1;
    }

    Color GetColor([[maybe_unused]] int led_idx) {
        assert(led_idx == 0);
        return current_color;
    }

    void SetColor([[maybe_unused]] int led_idx, Color color) {
        assert(led_idx == 0);
        SetColor(color);
    }

    void SetColor(Color color) {
        if (color == current_color) {
            return;
        }

        current_color = color;
        needs_update = true;
    }

#if XL_ENCLOSURE_SUPPORT()
    void SetEnclosureFanPwm(uint8_t pwm) {
        if (enclosure_fan_pwm != pwm) {
            enclosure_fan_pwm = pwm;
            needs_update = true;
        }
    }
#endif // XL_ENCLOSURE_SUPPORT

    void Update();

private:
    Color current_color;
    std::atomic<uint8_t> enclosure_fan_pwm;
    std::atomic<bool> needs_update = false;

#if PRINTER_IS_PRUSA_XL()
    /// First driver in the daisy chain: RGB, second driver: W + enclosure fan
    static constexpr size_t led_drivers_count = 2;
#elif PRINTER_IS_PRUSA_iX()
    /// 3x 3 or 3 x 6 RGB drivers in the U shape along the gantry (left, back, right)
    /// Newer strips have double the segments (the 3 x 6 version), just
    /// unconditionally send data for the variant with more segments
    static constexpr size_t led_drivers_count = 18;
#else
    #error "Not defined for this printer."
#endif

    using Leds = neopixel::SPI_10M5Hz<led_drivers_count, SideStripWriter::write>;
    Leds leds;
};

extern SideStrip side_strip;

} // namespace leds
