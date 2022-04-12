#include "../PrusaGcodeSuite.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M70X.hpp"

/**
 * M707: Read variable from MMU
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  A<address>  - address of variable
 *  C<size>     - size in bytes
 *
 *  Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M707() {
}

/**
 * M708: Write variable to MMU
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  A<address>  - address of variable
 *  C<size>     - size in bytes
 *
 *  Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M708() {
}

/**
 * M709: MMU turn on/off/reset
 *
 *  T<extruder>   - Extruder number. Required for mixing extruder.
 *                  For non-mixing, current extruder if omitted.
 *  X<reset type> - 0 command via communication into the MMU (soft reset)
 *                - 1 hard reset via MMU's reset pin
 *                - 2 power cycle reset
 *  Sn<power>     - 0 turn off MMU's power supply
 *                - 1 power up the MMU after being turned off
 *                - without any parameter returns 0 or 1 for current state
 *
 *  Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M709() {
    const int16_t reset_val = parser.byteval('X', -1);
    const int16_t power_val = parser.byteval('S', -1);

    switch (power_val) {
    case -1:
        //TODO return if is on
        return;
    case 0:
        filament_gcodes::mmu_off();
        return;
    case 1:
        filament_gcodes::mmu_on();
        return;
    }

    // check if value is in uint8_t range
    if (reset_val == uint8_t(reset_val)) {
        filament_gcodes::mmu_reset(reset_val);
        return;
    }
}

__attribute__((weak)) void filament_gcodes::mmu_reset(uint8_t) {}
__attribute__((weak)) void filament_gcodes::mmu_on() {}
__attribute__((weak)) void filament_gcodes::mmu_off() {}
