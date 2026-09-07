/// @file
#pragma once

namespace indx_head_leds {

/// (Re)evaluates the printer state and pushes the matching color/animation to the
/// INDX head LEDs, but only when the resulting state actually changes — the head
/// runs the animations itself, so buddy just reacts to transitions.
///
/// Respects config_store().tool_leds_enabled. Follows the chamber light: shares its
/// brightness levels and dims on the same inactivity (alert states stay lit). Called
/// from LEDManager::update() (rate-limited, power-panic aware).
void update();

} // namespace indx_head_leds
