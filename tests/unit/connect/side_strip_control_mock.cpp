#include <leds/side_strip_handler.hpp>

namespace leds {

SideStripHandler::SideStripHandler() {}

SideStripHandler &SideStripHandler::instance() {
    static SideStripHandler instance;
    return instance;
}

void SideStripHandler::activity_ping() {
}

uint8_t side_max_brightness = 0;
void SideStripHandler::set_max_brightness(uint8_t set) {
    side_max_brightness = set;
}

uint8_t side_print_brightness = 0;
void SideStripHandler::set_print_brightness(uint8_t set) {
    side_print_brightness = set;
}

} // namespace leds
