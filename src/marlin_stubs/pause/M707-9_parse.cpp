#include "marlin_stubs/PrusaGcodeSuite.hpp"
#include "Marlin/src/gcode/gcode.h"
#include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#include "M70X.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 * Read variable from MMU and print on the serial line.
 *
 * ## Parameters
 *
 * - `A` - Address of register to read from in hexadecimal.
 */
void PrusaGcodeSuite::M707() {
    if (!parser.seen('A')) {
        parser.unknown_command_error();
        return;
    }

    const uint32_t address = parser.ulongval_hex('A');
    MMU2::mmu2.ReadRegister(address);
}

/**
 * @brief Write variable to MMU
 *
 * ## Parameters
 *
 * - `A` - Address of register to write to in hexadecimal.
 * - `X` - The value to write (in decimal).
 */
void PrusaGcodeSuite::M708() {
    if (!parser.seen('A') || !parser.seen('X')) {
        parser.unknown_command_error();
        return;
    }

    const uint32_t address = parser.ulongval_hex('A');
    const uint16_t value = parser.intval('X');
    MMU2::mmu2.WriteRegister(address, value);
}

/**
 * @brief MMU turn on/off/reset
 *
 * ## Parameters
 *
 * -`T` - Extruder number. Required for mixing extruder.
 *        For non-mixing, current extruder if omitted.
 * -`X` - reset type
 *      - 0 command via communication into the MMU (soft reset)
 *      - 1 hard reset via MMU's reset pin
 *      - 2 power cycle reset
 * -`S` - power off/on
 *      - 0 turn off MMU's power supply
 *      - 1 power up the MMU after being turned off
 *
 *  without any parameter returns 0 or 1 for current state
 *
 * Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M709() {
    const int16_t reset_val = parser.byteval('X', -1);
    const int16_t power_val = parser.byteval('S', -1);

    switch (power_val) {
    case -1:
        // TODO return if is on
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

/** @}*/

__attribute__((weak)) void filament_gcodes::mmu_reset(uint8_t) {}
__attribute__((weak)) void filament_gcodes::mmu_on() {}
__attribute__((weak)) void filament_gcodes::mmu_off() {}
