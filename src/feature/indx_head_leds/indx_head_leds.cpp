/// @file
#include "indx_head_leds.hpp"

#include <puppies/INDX.hpp>
#include <leds/status_leds_handler.hpp>
#include <leds/side_strip_handler.hpp>
#include <indx_head/leds.hpp>
#include <config_store/store_instance.hpp>
#include <utils/color.hpp>
#include <utils/enum_array.hpp>

#include <cstdint>
#include <optional>

namespace indx_head_leds {

namespace {

    using indx_head::leds::Mode;
    using leds::StateAnimation;

    struct LedSetting {
        Color color;
        Mode mode;
        uint16_t period_ms;
    };

    static constexpr LedSetting off = { .color = Color::from_rgb(0, 0, 0), .mode = Mode::solid, .period_ms = 500 };
    static constexpr LedSetting red = { .color = Color::from_rgb(127, 0, 0), .mode = Mode::solid, .period_ms = 500 };
    static constexpr LedSetting green = { .color = Color::from_rgb(0, 127, 0), .mode = Mode::solid, .period_ms = 500 };
    static constexpr LedSetting blue = { .color = Color::from_rgb(0, 0, 127), .mode = Mode::solid, .period_ms = 500 };

    constexpr EnumArray<StateAnimation, LedSetting, static_cast<int>(StateAnimation::_last) + 1> palette {
        { StateAnimation::Idle, off },
        { StateAnimation::Printing, blue },
        { StateAnimation::Finished, green },
        { StateAnimation::Aborted, off },
        { StateAnimation::Warning, red },
        { StateAnimation::PowerPanic, red },
        { StateAnimation::PowerUp, green },
        { StateAnimation::Error, red },
    };

    constexpr Color color_off = Color::from_rgb(0, 0, 0);

    // Alert states stay lit at full brightness even while the chamber is dimmed,
    // so a problem remains visible when nobody is interacting with the printer.
    constexpr bool is_alert(StateAnimation state) {
        return state == StateAnimation::Warning
            || state == StateAnimation::Error
            || state == StateAnimation::PowerPanic;
    }

    constexpr Color scale(Color c, uint8_t brightness) {
        return Color::from_rgb((c.r * brightness) / 255, (c.g * brightness) / 255, (c.b * brightness) / 255);
    }

    struct HeadState {
        std::optional<StateAnimation> animation;
        uint8_t brightness {};
        constexpr bool operator==(const HeadState &) const = default;
    };

    HeadState compute_state() {
        if (!config_store().tool_leds_enabled.get()) {
            return {};
        }

        const auto animation = leds::StatusLedsHandler::instance().current_animation();

        // Follow the chamber light: share its two brightness levels and dim on the same inactivity.
        auto &chamber = leds::SideStripHandler::instance();
        const uint8_t brightness = chamber.is_dimmed() && !is_alert(animation)
            ? chamber.get_dimmed_brightness()
            : chamber.get_max_brightness();

        return { animation, brightness };
    }

    void apply_state(const HeadState &state) {
        auto &indx = buddy::puppies::indx;
        if (!state.animation.has_value()) {
            indx.set_leds_enabled(false);
            return;
        }

        const LedSetting &setting = palette[*state.animation];
        const Color color = scale(setting.color, state.brightness);
        switch (setting.mode) {
        case Mode::off:
            indx.set_leds_enabled(false);
            break;
        case Mode::solid:
            indx.set_leds_solid_color(color, setting.period_ms);
            break;
        case Mode::blinking:
            indx.set_leds_blinking(color, color_off, setting.period_ms);
            break;
        case Mode::pulsing:
            indx.set_leds_pulsing(color, color_off, setting.period_ms);
            break;
        case Mode::match_nozzle_temp:
            indx.set_leds_to_follow_nozle_temp();
            break;
        }
    }

} // namespace

void update() {
    // LEDManager::update() already rate-limits us; only push to the head on a real change.
    static HeadState last_state;

    const auto state = compute_state();
    if (last_state == state) {
        return;
    }
    last_state = state;
    apply_state(state);
}

} // namespace indx_head_leds
