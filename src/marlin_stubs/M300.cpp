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
 *### M300: Play beep sound <a href="https://reprap.org/wiki/G-code#M300:_Play_beep_sound">M300: Play beep sound</a>
 *
 *#### Usage
 *
 *    M300 [ S | P | V ]
 *
 *#### Parameters
 *
 *  - `S` - frequency Hz
 *  - `P` - duration ms
 *  - `V` - volume 0.0 to 1.0
 *    - `0` - no sound played
 *    - `1` - max volume
 *
 *#### Example
 *
 *    M300 P440 P2000 V1   ; Play 440Hz sound for 2000 ms / 2 seconds with 100% volume
 *    M300 P440 P2000 V0.5 ; Play 440Hz sound for 2000 ms / 2 seconds with 50% volume
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
