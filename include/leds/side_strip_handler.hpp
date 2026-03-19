#pragma once

#include "color.hpp"
#include "printers.h"

#include <freertos/mutex.hpp>
#include <optional>
#include <option/has_xbuddy_extension.h>

namespace leds {

enum class SideStripState {
    off,
    dimmed, // idle brightness (not printing, after activity timeout)
    active, // interaction brightness (door open, knob, touch)
    printing, // print brightness (printing, after activity timeout)
    custom_color,
    _last = custom_color,
};

class SideStripHandler {
public:
    static constexpr bool has_white_led_and_enclosure_on_second_driver() {
#if PRINTER_IS_PRUSA_XL()
        return true;
#else
        return false;
#endif
    }

    static constexpr bool has_white_led() {
#if PRINTER_IS_PRUSA_COREONE()
        return true;
#else
        return has_white_led_and_enclosure_on_second_driver();
#endif
    }

    static SideStripHandler &instance();

    SideStripHandler();

    void activity_ping();

    void set_custom_color(ColorRGBW color, uint32_t duration_ms, uint32_t transition_ms);

    void update();

    void load_config();

    /// Brightness on user interaction (door open, knob, touch). 0 = off, 255 = full.
    uint8_t get_max_brightness() const;
    void set_max_brightness(uint8_t value);

    /// Brightness when idle and not printing (after activity timeout). 0 = off, 255 = full.
    uint8_t get_dimmed_brightness() const;
    void set_dimmed_brightness(uint8_t value);

    /// Brightness during active printing (after activity timeout). 0 = off, 255 = full.
    uint8_t get_print_brightness() const;
    void set_print_brightness(uint8_t value);

    leds::ColorRGBW color() const;

private:
    static constexpr uint32_t active_timeout_ms = 120 * 1000;

    void change_state(SideStripState state);

    ColorRGBW get_color_for_state(SideStripState state);

    struct CustomColorState {
        ColorRGBW color;
        uint32_t start_ms;
        uint32_t duration_ms;
        uint32_t transition_ms;
    };

    mutable freertos::Mutex mutex;

    // Values are initialized from config store by load_config() in constructor
    uint8_t max_brightness;
    uint8_t dimmed_brightness;
    uint8_t print_brightness;

    SideStripState state = SideStripState::off;
    uint32_t active_timestamp_ms = 0; // Timestamp of the last activity for idle dimming
    std::optional<CustomColorState> custom_color;
};

} // namespace leds
