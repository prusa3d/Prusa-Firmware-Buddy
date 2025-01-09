#include <leds/side_strip_control.hpp>

namespace leds {

SideStripControl side_strip_control;

uint8_t side_max_brightness = 0;

void SideStripControl::ActivityPing() {
}

void SideStripControl::set_max_brightness(uint8_t set) {
    side_max_brightness = set;
}

} // namespace leds
