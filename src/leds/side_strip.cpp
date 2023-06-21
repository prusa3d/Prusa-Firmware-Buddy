#include <leds/side_strip.hpp>
#include <device/hal.h>
#include <device/peripherals.h>
#include <cmsis_os.h>
#include "neopixel.hpp"

using namespace leds;

SideStrip leds::side_strip;

void SideStrip::Update() {
    if (!needs_update)
        return;
    needs_update = false;

    leds.Set(Color(current_color.g, current_color.r, current_color.b).data, 1);
    leds.Set(Color(0, current_color.w, 0).data, 0);
    leds.ForceRefresh(2);
    leds.Send();
}
