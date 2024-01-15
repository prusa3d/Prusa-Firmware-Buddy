/**
 * @file
 */
#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "../lib/Marlin/Marlin/src/gcode/gcode.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 * Play beep sound
 *
 * ## Parameters
 *  - `S` - frequency Hz
 *  - `P` - duration ms
 *  - `V` - volume
 */
void PrusaGcodeSuite::M300() {
    uint16_t const frequency = parser.ushortval('S', 100);
    uint16_t duration = parser.ushortval('P', 1000);
    float volume = parser.floatval('V', 1);

    // Limits the tone duration to 0-5 seconds.
    NOMORE(duration, 5000U);
    Sound::getInstance().singleSound(frequency, duration, volume);
    GcodeSuite::dwell(duration);
}

/** @}*/
