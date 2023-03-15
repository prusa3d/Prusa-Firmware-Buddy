#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "led_animations/led_types.h"
#include "led_animations/printer_animation_state.hpp"
#include "led_animations/animation.hpp"
#include <optional>

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
 * M150: Set led animations
 *
 * A[id] - animation type
 * S[id[ - printer state
 *
 * Color input supports RGB and HSV format
 *
 * R[value] - from 0 to 255
 * G[value]
 * B[value]
 *
 * H[value] from 0 to 360
 * S[value] from 0 to 100
 * V[value] form 0 to 100
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
