#pragma once
#include "led_strip.hpp"
#include <assert.h>
#include <hw/neopixel.hpp>

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

    static void SendLedData(uint8_t *pb, uint16_t size);

    static constexpr uint32_t T1H_10M5Hz = 7;   // 95.24 * 7 = 666.68 ns
    static constexpr uint32_t T1L_10M5Hz = 3;   // 95.24 * 3 = 285.72 ns
    static constexpr uint32_t T0H_10M5Hz = 3;   // 95.24 * 3 = 285.72 ns
    static constexpr uint32_t T0L_10M5Hz = 7;   // 95.24 * 7 = 666.68 ns
    static constexpr uint32_t RESET_10M5Hz = 5; // 95.24 * 5 = 476.2 ns
                                                //
    using Leds = neopixel::LedsSPI<2, SideStrip::SendLedData, T1H_10M5Hz, T1L_10M5Hz, T0H_10M5Hz, T0L_10M5Hz, RESET_10M5Hz>;

    Leds leds;
};

extern SideStrip side_strip;

}
