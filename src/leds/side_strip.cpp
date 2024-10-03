#include <leds/side_strip.hpp>
#include <device/hal.h>
#include <device/peripherals.h>
#include <cmsis_os.h>
#include "neopixel.hpp"

using namespace leds;

SideStrip leds::side_strip;

void SideStrip::Update() {
    if (!needs_update) {
        return;
    }
    needs_update = false;

    // !!! The indexes here are INVERSED compared to actual LED driver daisy-chain order
    // because of wrong neopixel.hpp::LedsSPI_MSB implementation
    // BFW-5067

    size_t i = 0;

    // White led -> there are two daisy-chained drivers, the first one is RGB, the other one is W (+ XL enclosure fan or whatevs)
    // BFW-5067: The led/fan control driver is actually second in the daisy-chain (but the indexing in the code is inverted)
    static_assert(!HasWhiteLed() || led_drivers_count == 2);
    if (HasWhiteLed()) {
        leds.Set(ColorRGBW(enclosure_fan_pwm, current_color.w, 0).data, 0);
        i++;
    }

    for (; i < led_drivers_count; ++i) {
        leds.Set(ColorRGBW(current_color.g, current_color.r, current_color.b).data, i);
    }

    leds.Tick();
}
