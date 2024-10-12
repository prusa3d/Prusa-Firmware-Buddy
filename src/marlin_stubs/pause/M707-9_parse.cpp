#include "marlin_stubs/PrusaGcodeSuite.hpp"
#include "Marlin/src/gcode/gcode.h"
#include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#include "M70X.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M707 - Read from MMU register <a href="https://reprap.org/wiki/G-code#M707:_Read_from_MMU_register">M707: Read from MMU register</a>
 *
 *#### Usage
 *
 *    M707 [ A ]
 *
 *#### Parameters
 *
 * - `A` - Address of register in hexidecimal.
 *
 *#### Example
 *
 *    M707 A0x1b - Read a 8bit integer from register 0x1b and prints the result onto the serial line.
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
 *### M708 - Write to MMU register <a href="https://reprap.org/wiki/G-code#M708:_Write_to_MMU_register">M707: Write to MMU register</a>
 *
 *#### Usage
 *
 *    M708 [ A | X ]
 *
 *#### Parameters
 *
 * - `A` - Address of register in hexidecimal.
 * - `X` - Data to write (16-bit integer). Default value 0.
 *
 *#### Example
 *
 *    M708 A0x1b X05 - Write to register 0x1b the value 05.
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
 * ### M709 - MMU power & reset <a href="https://reprap.org/wiki/G-code#M709:_MMU_power_&_reset">M709: MMU power & reset</a>
 *
 *#### Usage
 *
 *    M709 [ S | X ]
 *
 * #### Parameters
 *
 * - `X` - reset type
 *   - `0` - command via communication into the MMU (soft reset)
 *   - `1` - hard reset via MMU's reset pin
 *   - `2` - power cycle reset
 *   - `42` - erase MMU eeprom
 * - `S` - power off/on
 *   - `0`- turn off MMU's power supply
 *   - `1` - power up the MMU after being turned off
 *
 *  without any parameter returns 0 or 1 for current state
 *
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
