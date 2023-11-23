#include "led.h"
#include "disable_interrupts.h"
#include "hwio_pindef.h"
#include "timing_precise.hpp"
#include "timing.h"

using namespace buddy::hw;

namespace led {
constexpr auto time_short_ns { 220 }; // doc: ns 220 - 0:380/1:420
constexpr auto time_long_ns { 580 }; // doc: ns 580 - 1000
constexpr auto time_reset_us { 280 }; // doc: us 280+

FORCE_INLINE void send_zero() {
    neopixel.set();
    timing_delay_cycles(timing_nanoseconds_to_cycles(time_short_ns));
    neopixel.reset();
    timing_delay_cycles(timing_nanoseconds_to_cycles(time_long_ns));
}

FORCE_INLINE void send_one() {
    neopixel.set();
    timing_delay_cycles(timing_nanoseconds_to_cycles(time_long_ns));
    neopixel.reset();
    timing_delay_cycles(timing_nanoseconds_to_cycles(time_short_ns));
}

FORCE_INLINE void send_reset() {
    neopixel.reset();
    timing_delay_cycles(timing_microseconds_to_cycles(time_reset_us));
}

#pragma GCC push_options
#pragma GCC optimize("O3")
void set_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    //  Requires heavy optimizations because the timings are strict - all called functions need to be inlined, otherwise the pragma O3 won't be applied to them.
    send_reset();
    const uint32_t col = (green << 16) | (red << 8) | blue; // concat the colors
    {
        buddy::DisableInterrupts dis;
        for (int i = 0; i < 24; ++i) {
            if (col & (1 << (23 - i))) { // if bit set (from highest bit to lowest)
                send_one();
            } else {
                send_zero();
            }
        }
    }
}
#pragma GCC pop_options

namespace {

    /**
     * @brief Update loop start.
     * @param period loop period [ms]
     * @return ticks since current loop start
     */
    uint32_t sync_tick_start(uint32_t period) {
        static uint32_t loop_tick_start_ms { 0 }; ///< Remember when ticks start to ease on division
        auto ticks_now_ms = ticks_ms();

        if (ticks_now_ms > loop_tick_start_ms + period) { // Ticker overflow
            loop_tick_start_ms = ticks_now_ms - ticks_now_ms % (period); // Synchronized flashing start
        }

        return ticks_now_ms - loop_tick_start_ms; // Ticks since current loop start
    }

} // namespace

void blinking(uint8_t red, uint8_t green, uint8_t blue, uint32_t on_duration_ms, uint32_t off_duration_ms) {
    auto ticks = sync_tick_start(on_duration_ms + off_duration_ms);

    static bool current_state { false };
    bool required_state = ticks < on_duration_ms;
    if (required_state != current_state) {
        if (required_state) { // On
            set_rgb(red, green, blue);
            current_state = true;
        } else { // Off
            set_rgb(0, 0, 0);
            current_state = false;
        }
    }
}

void pulsing(uint8_t red0, uint8_t green0, uint8_t blue0, uint8_t red1, uint8_t green1, uint8_t blue1, uint32_t period_ms) {
    auto ticks = sync_tick_start(period_ms);

    uint32_t power; // Color 0 power [0 - 0x100]
    if (ticks < period_ms / 2) { // Going on
        power = ticks * 2 * 0x100 / period_ms;
    } else { // Going off
        power = (period_ms - ticks) * 2 * 0x100 / period_ms;
    }
    uint32_t antipower = 0x100 - power; // Color 1 power [0 - 0x100]

    set_rgb((red0 * power + red1 * antipower) >> 8,
        (green0 * power + green1 * antipower) >> 8,
        (blue0 * power + blue1 * antipower) >> 8);
}

}; // namespace led
