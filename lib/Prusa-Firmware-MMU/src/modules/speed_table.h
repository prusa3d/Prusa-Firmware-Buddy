/// @file speed_table.h
#pragma once
#include "../config/config.h"
#include "../hal/progmem.h"
#include "../hal/cpu.h"
#include "math.h"

namespace modules {

/// Speed  tables for acceleration calculations
namespace speed_table {

typedef uint16_t st_timer_t;

/// CPU timer frequency divider required for the speed tables
static_assert(F_CPU / config::stepTimerFrequencyDivider == 2000000,
    "speed tables not compatible for the requested frequency");

/// Lookup table for rates equal or higher than 8*256
extern const st_timer_t speed_table_fast[256][2] PROGMEM;

/// Lookup table for lower step rates
extern const st_timer_t speed_table_slow[256][2] PROGMEM;

/// Calculate the next timer interval and steps according to current step rate
static inline st_timer_t calc_timer(st_timer_t step_rate, uint8_t &step_loops) {
    if (step_rate > config::maxStepFrequency)
        step_rate = config::maxStepFrequency;
    if (step_rate > 20000) { // If steprate > 20kHz >> step 4 times
        step_rate = (step_rate >> 2) & 0x3fff;
        step_loops = 4;
    } else if (step_rate > 10000) { // If steprate > 10kHz >> step 2 times
        step_rate = (step_rate >> 1) & 0x7fff;
        step_loops = 2;
    } else {
        step_loops = 1;
    }

    using modules::math::mulU8X16toH16;
    namespace pm = hal::progmem;

    st_timer_t timer; // calculated interval

    if (step_rate < (F_CPU / 500000))
        step_rate = (F_CPU / 500000);
    step_rate -= (F_CPU / 500000); // Correct for minimal speed
    if (step_rate >= (8 * 256)) { // higher step rate
        const uint16_t *table_address = &speed_table_fast[(uint8_t)(step_rate >> 8)][0];
        uint8_t tmp_step_rate = (step_rate & 0x00ff);
        uint16_t gain = pm::read_word(table_address + 1);
        timer = mulU8X16toH16(tmp_step_rate, gain);
        timer = pm::read_word(table_address) - timer;
    } else { // lower step rates
        const uint16_t *table_address = &speed_table_slow[0][0];
        table_address += (step_rate >> 2) & 0xfffe;
        timer = pm::read_word(table_address);
        timer -= ((pm::read_word(table_address + 1) * (uint8_t)(step_rate & 0x0007)) >> 3);
    }
    if (timer < 100) {
        // 20kHz this should never happen
        timer = 100;
    }
    return timer;
}

} // namespace speed_table
} // namespace modules
