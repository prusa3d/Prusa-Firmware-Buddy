/**
 * @file screen_menu_move_utils.cpp
 */

#include "screen_menu_move_utils.hpp"

#include "menu_vars.h"
#include "marlin_client.hpp"

void jog_axis(float &position, const float target, const AxisEnum axis) {
    if (position == target) {
        // Yeah and every time I try to go where I really want to be
        // it's already where I am 'cause I'm already there!
        return;
    }

    // This empirical constant was carefully crafted in such
    // a clever way that it seems to work most of the time.
    constexpr float magic_constant = 1. / (BLOCK_BUFFER_SIZE * 60 * 1.25 * 5);
    const float feedrate = MenuVars::GetManualFeedrate()[axis];
    const float short_segment = feedrate * magic_constant;
    const float long_segment = 5 * short_segment;

    // Just fill the entire queue with movements.
    // When i went up to BLOCK_BUFFER_SIZE, it was still choppy in certain situations
    for (uint8_t i = marlin_vars()->pqueue; i < BLOCK_BUFFER_SIZE - 1; i++) {
        const float difference = (float)target - position;
        if (difference == 0) {
            break;
        } else if (difference >= long_segment) {
            position += long_segment;
        } else if (difference >= short_segment) {
            position += short_segment;
        } else if (difference <= -long_segment) {
            position -= long_segment;
        } else if (difference <= -short_segment) {
            position -= short_segment;
        } else {
            position = target;
        }
        marlin_client::move_axis(position, feedrate, axis);
    }
}
