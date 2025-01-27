/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "led_animations/led_types.h"
#include "led_animations/printer_animation_state.hpp"
#if HAS_SIDE_LEDS()
    #include "leds/side_strip_control.hpp"
#endif
#include <optional>
#include <gui/led_animations/animation_model.hpp>

std::optional<leds::Color> parse_color() {
    if (parser.seen('R') && parser.seen('G') && parser.seen('B')) {
        uint8_t R = parser.byteval('R');
        uint8_t G = parser.byteval('G');
        uint8_t B = parser.byteval('B');
        return leds::Color { R, G, B };
    } else if (parser.seen('S') && parser.seen('H') && parser.seen('V')) {
        float H = parser.floatval('H');
        float S = parser.floatval('S');
        float V = parser.floatval('V');
        return leds::Color { leds::HSV(H, S, V) };
    }
    return std::nullopt;
}

/**
 * \addtogroup G-Codes
 */

/**
 * @brief Set display led animations
 *
 * Color input supports RGB and HSV format
 *
 * ## Parameters
 *
 * ### RGB color space
 *  - `R` - Red intensity from 0 to 255
 *  - `G` - Green intensity from 0 to 255
 *  - `B` - Blue intensity from 0 to 255
 *
 * ### HSV color space
 *  - `H` - Hue from 0 to 360
 *  - `S` - Saturation from 0 to 100
 *  - `V` - Saturation form 0 to 100
 *
 * ### Effect
 *  - `A` - animation type
 *  - `S` - printer state
 */
void PrusaGcodeSuite::M150() {
    if (parser.seen('A') && parser.seen('S')) {
        uint8_t animation = parser.byteval('A');
        uint16_t state = parser.byteval('S');
        if (animation > static_cast<uint8_t>(AnimationTypes::_count) || state > static_cast<uint16_t>(PrinterState::_count)) {
            return;
        }
        auto color = parse_color();
        if (color) {
            auto color_val = color.value();
            uint16_t period = parser.ushortval('P', 0);
            PrinterStateAnimation::set_animation(static_cast<PrinterState>(state), Animation_model { animation, color_val.r, color_val.g, color_val.b, period, 0, 0 });
        }
    }
}

/**
 * @brief Set side strip
 *
 * Color input supports RGB and HSV format
 *
 * ## Parameters
 *
 * ### RGB color space
 *  - `R` - Red intensity from 0 to 255
 *  - `G` - Green intensity from 0 to 255
 *  - `B` - Blue intensity from 0 to 255
 *
 * ### HSV color space
 *  - `H` - Hue from 0 to 360
 *  - `S` - Saturation from 0 to 100
 *  - `V` - Saturation form 0 to 100
 *
 * ### Effect
 *  - `D` - duration in milliseconds, iX only: set to 0 for infinite duration
 *  - `T` - transition in milliseconds (fade in / fade out)
 *
 * Fade in is counted toward duration,
 * so if duration is greater than 0 and less than transition,
 * effect doesn't reach full color intensity.
 * Fade out is not counted toward duration.
 */
#if HAS_SIDE_LEDS()
void PrusaGcodeSuite::M151() {
    auto color = parse_color();
    if (color) {
        auto color_val = color.value();
        uint32_t duration = parser.ulongval('D', 400);
        uint32_t transition = parser.ulongval('T', 100);
        leds::side_strip_control.PresentColor(color_val, duration, transition);
    }
}

/** @}*/

#endif
