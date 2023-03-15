#include "led.h"
#include "disable_interrupts.h"
#include "hwio_pindef.h"
#include "timing_precise.hpp"
#include "timing.h"

using namespace buddy::hw;

namespace led {
constexpr auto time_short_ns { 220 }; // doc: ns 220 - 0:380/1:420
constexpr auto time_long_ns { 580 };  // doc: ns 580 - 1000
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

void blinking(uint8_t red, uint8_t green, uint8_t blue, uint32_t on_duration_ms, uint32_t off_duration_ms) {
    static bool direction_on { false };
    static uint32_t loop_tick_start_ms { 0 };

    if (auto ticks_now_ms = ticks_ms(); ticks_now_ms < loop_tick_start_ms) { // ticker overflow
        loop_tick_start_ms = ticks_now_ms;
    } else if (auto diff = ticks_now_ms - loop_tick_start_ms; direction_on && diff > on_duration_ms) { // go to off
        set_rgb(0, 0, 0);
        direction_on = false;
        loop_tick_start_ms = ticks_now_ms;
    } else if (!direction_on && diff > off_duration_ms) { // go to on
        set_rgb(red, green, blue);
        direction_on = true;
        loop_tick_start_ms = ticks_now_ms;
    } // else nothing needs to be done
}

};
