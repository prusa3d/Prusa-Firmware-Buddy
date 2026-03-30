#include "leds/side_strip_handler.hpp"
#include "leds/simple_transition_controller.hpp"
#include "marlin_server.hpp"

namespace leds {

static SimpleTransitionController &controller_instance() {
    static SimpleTransitionController instance;
    return instance;
}

SideStripHandler &SideStripHandler::instance() {
    static SideStripHandler instance;
    return instance;
}

SideStripHandler::SideStripHandler() {
    load_config();
}

void SideStripHandler::activity_ping() {
    std::lock_guard lock(mutex);
    active_timestamp_ms = ticks_ms();
}

void SideStripHandler::set_custom_color(ColorRGBW color, uint32_t duration_ms, uint32_t transition_ms) {
    std::lock_guard lock(mutex);
    custom_color.emplace(color, ticks_ms(), duration_ms, transition_ms);
    // Set state temporarily to off to force a change in case the state is already custom_color
    state = SideStripState::off;
}

void SideStripHandler::update() {
    std::lock_guard lock(mutex);
    const uint32_t time_ms = ticks_ms();

    auto &controller = controller_instance();
    if (custom_color && time_ms - custom_color->start_ms < custom_color->duration_ms) {
        change_state(SideStripState::custom_color);
    } else {
        custom_color.reset();
        const bool is_printing_now = marlin_server::is_printing();
        const uint8_t baseline = is_printing_now ? print_brightness : dimmed_brightness;
        const bool user_active = (active_timestamp_ms != 0 && time_ms - active_timestamp_ms < active_timeout_ms);

        if (user_active && max_brightness > baseline) {
            change_state(SideStripState::active);
        } else {
            // Clear to prevent overflow bugs
            active_timestamp_ms = 0;
            if (is_printing_now) {
                change_state(SideStripState::printing);
            } else {
                change_state(SideStripState::dimmed);
            }
        }
    }

    controller.update();
}

void SideStripHandler::load_config() {
    std::lock_guard lock(mutex);
    dimmed_brightness = config_store().side_leds_dimmed_brightness.get();
    max_brightness = config_store().side_leds_max_brightness.get();
    print_brightness = config_store().side_leds_print_brightness.get();
    // Enforce invariant: idle brightness must not exceed chamber brightness.
    // Persisted configs or connect-driven changes may violate this, so clamp here.
    if (dimmed_brightness > max_brightness) {
        dimmed_brightness = max_brightness;
        config_store().side_leds_dimmed_brightness.set(dimmed_brightness);
    }
    // Set state to off to force a change of state that will transition to the new brightness
    state = SideStripState::off;
}

uint8_t SideStripHandler::get_max_brightness() const {
    std::lock_guard lock(mutex);
    return max_brightness;
}

void SideStripHandler::set_max_brightness(uint8_t value) {
    config_store().side_leds_max_brightness.set(value);
    std::lock_guard lock(mutex);
    if (max_brightness == value) {
        return;
    }

    max_brightness = value;
    // Clamp idle brightness down if it now exceeds the new max
    if (dimmed_brightness > max_brightness) {
        dimmed_brightness = max_brightness;
        config_store().side_leds_dimmed_brightness.set(dimmed_brightness);
    }
    // Set state to off to force a change of state that will transition to the new brightness
    state = SideStripState::off;
}

uint8_t SideStripHandler::get_dimmed_brightness() const {
    std::lock_guard lock(mutex);
    return dimmed_brightness;
}

void SideStripHandler::set_dimmed_brightness(uint8_t value) {
    std::lock_guard lock(mutex);
    // Enforce invariant: idle brightness must not exceed chamber brightness
    if (value > max_brightness) {
        value = max_brightness;
    }
    config_store().side_leds_dimmed_brightness.set(value);
    if (dimmed_brightness == value) {
        return;
    }

    dimmed_brightness = value;
    // Set state to off to force a change of state that will transition to the new brightness
    state = SideStripState::off;
}

uint8_t SideStripHandler::get_print_brightness() const {
    std::lock_guard lock(mutex);
    return print_brightness;
}

void SideStripHandler::set_print_brightness(uint8_t value) {
    config_store().side_leds_print_brightness.set(value);
    std::lock_guard lock(mutex);
    if (print_brightness == value) {
        return;
    }

    print_brightness = value;
    // Set state to off to force a change of state that will transition to the new brightness
    state = SideStripState::off;
}

leds::ColorRGBW SideStripHandler::color() const {
    std::lock_guard lock(mutex);
    return controller_instance().color();
}

void SideStripHandler::change_state(SideStripState state) {
    if (this->state != state) {
        this->state = state;

        uint32_t duration;
        if (state == SideStripState::dimmed) {
            duration = 5000;
        } else if (state == SideStripState::custom_color) {
            duration = custom_color->transition_ms;
        } else {
            duration = 500;
        }
        controller_instance().set(get_color_for_state(state), duration);
    }
}

ColorRGBW SideStripHandler::get_color_for_state(SideStripState state) {
    constexpr auto base_color = has_white_led() ? ColorRGBW(0, 0, 0, 255) : ColorRGBW(255, 255, 255);

    switch (state) {
    case SideStripState::off:
        return ColorRGBW();
    case SideStripState::dimmed:
        return base_color.clamp(dimmed_brightness);
    case SideStripState::active:
        return base_color.clamp(max_brightness);
    case SideStripState::printing:
        return base_color.clamp(print_brightness);
    case SideStripState::custom_color:
        return ColorRGBW(custom_color->color);
    }
    return {};
}

} // namespace leds
