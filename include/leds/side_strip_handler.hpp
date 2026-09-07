#pragma once

#include <utils/led_color.hpp>
#include "printers.h"
#include "dimming_enabled.hpp"

#include <freertos/mutex.hpp>
#include <optional>
#include <option/has_xbuddy_extension.h>

namespace leds {

enum class SideStripState {
    off,
    dimmed,
    active,
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
#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
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

    /// Set maximum brightness of the side leds white channel. The W brightness is limited to this value.
    /// 0 = disable side leds completely (even RGB status blinking), 255 = full
    uint8_t get_max_brightness() const;
    void set_max_brightness(uint8_t value);
    /// Set brightness of the side leds while the leds are dimmed down.
    // /// 0 = disable dimmed side leds, 255 = full brightness
    uint8_t get_dimmed_brightness() const;
    void set_dimmed_brightness(uint8_t value);

    DimmingEnabled get_dimming_enabled() const;
    void set_dimming_enabled(DimmingEnabled value);

    leds::ColorRGBW color() const;

    /// True while the strip is dimmed down due to inactivity.
    /// Honours the dimming mode (Never/Always/On Idle), the inactivity timer and the printing state.
    bool is_dimmed() const;

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
    DimmingEnabled dimming_enabled;
    uint8_t max_brightness;
    uint8_t dimmed_brightness;

    SideStripState state = SideStripState::off;
    uint32_t active_timestamp_ms = 0; // Timestamp of the last activity for idle dimming
    std::optional<CustomColorState> custom_color;
};

} // namespace leds
