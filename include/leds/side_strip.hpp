#pragma once
#include "led_strip.hpp"
#include <assert.h>
#include <hw/neopixel.hpp>
#include "led_lcd_cs_selector.hpp"

namespace leds {

class SideStrip : public LedStrip {
public:
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

    static constexpr uint32_t T1H_10M5Hz = 7;   // 95.24 * 7 = 666.68 ns
    static constexpr uint32_t T1L_10M5Hz = 3;   // 95.24 * 3 = 285.72 ns
    static constexpr uint32_t T0H_10M5Hz = 3;   // 95.24 * 3 = 285.72 ns
    static constexpr uint32_t T0L_10M5Hz = 7;   // 95.24 * 7 = 666.68 ns
    static constexpr uint32_t RESET_10M5Hz = 5; // 95.24 * 5 = 476.2 ns

    using Leds = neopixel::SPI_10M5Hz<2, SideStripWriter::write>;
    Leds leds;
};

extern SideStrip side_strip;

}
