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

    for (size_t i = 0; i < led_drivers_count; ++i) {
        if (HasWhiteLed()) {
            leds.Set(Color(0, current_color.w, 0).data, i);
            ++i;
        }
        // swap colors
        leds.Set(Color(current_color.g, current_color.r, current_color.b).data, i);
    }

    leds.ForceRefresh(led_drivers_count);
    leds.Send();
}
