#pragma once
#include "led_strip.hpp"
#include <assert.h>
#include <hw/neopixel.hpp>
#include "led_lcd_cs_selector.hpp"
#include "printers.h"

namespace leds {

class SideStrip : public LedStrip {
public:
    static constexpr bool HasWhiteLed() {
#if PRINTER_IS_PRUSA_XL
        return true;
#elif PRINTER_IS_PRUSA_iX
        return false;
#else
    #error "Not defined for this printer."
#endif
    }

    SideStrip()
        : current_color()
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
        current_color = color;
        needs_update = true;
    }

    void Update();

private:
    Color current_color;
    bool needs_update = false;

#if PRINTER_IS_PRUSA_XL
    static constexpr size_t led_drivers_count = 2;
#elif PRINTER_IS_PRUSA_iX
    static constexpr size_t led_drivers_count = 9;
#else
    #error "Not defined for this printer."
#endif

    using Leds = neopixel::SPI_10M5Hz<led_drivers_count, SideStripWriter::write>;
    Leds leds;
};

extern SideStrip side_strip;

} // namespace leds
