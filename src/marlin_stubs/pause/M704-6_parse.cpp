/**
 * @file
 */
#include "../PrusaGcodeSuite.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M70X.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M704: Preload to MMU <a href="https://reprap.org/wiki/G-code#M704:_Preload_to_MMU">M704: Preload to MMU</a>
 *
 * Only MK3.5/S, MK3.9/S and MK4/S with MMU
 *
 *#### Usage
 *
 *    M704 [ P ]
 *
 *#### Parameters
 *
 * - `P` - MMU index of slot (zero based)
 *
 *#### Examples
 *
 *    M704 P0 ; Start preload procedure at slot 0
 */
void PrusaGcodeSuite::M704() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_load(val);
}

/**
 *### M1704: Test Load to MMU <a href=" "> </a>
 *
 * Only MK3.5/S, MK3.9/S and MK4/S with MMU
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1704 [ P ]
 *
 *#### Parameters
 *
 * - `P` - MMU index of slot (zero based)
 */
void PrusaGcodeSuite::M1704() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_load_test(val);
}

/**
 *### M705: Eject filament <a href="https://reprap.org/wiki/G-code#M705:_Eject_filament">M705: Eject filament</a>
 *
 * Only MK3.5/S, MK3.9/S and MK4/S with MMU
 *
 *#### Usage
 *
 *    M [ P ]
 *
 *#### Parameters
 *
 * - `P` - MMU index of slot (zero based)
 */
void PrusaGcodeSuite::M705() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_eject(val);
}

/**
 *### M706: Cut filament <a href="https://reprap.org/wiki/G-code#M706:_Cut_filament">M706: Cut filament</a>
 *
 * Only MK3.5/S, MK3.9/S and MK4/S with MMU
 *
 *#### Usage
 *
 *    M706 [ P ]
 *
 *#### Parameters
 *
 * - `P` - MMU index of slot (zero based)
 *
 */
void PrusaGcodeSuite::M706() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_cut(val);
}

/** @}*/

__attribute__((weak)) void filament_gcodes::mmu_load([[maybe_unused]] uint8_t data) {}
__attribute__((weak)) void filament_gcodes::mmu_eject([[maybe_unused]] uint8_t data) {}
__attribute__((weak)) void filament_gcodes::mmu_cut([[maybe_unused]] uint8_t data) {}
__attribute__((weak)) void filament_gcodes::mmu_load_test([[maybe_unused]] uint8_t data) {}
