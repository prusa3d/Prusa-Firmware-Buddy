#include "../PrusaGcodeSuite.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M70X.hpp"

/**
 * M704: Load filament to MMU
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  P<mmu>      - MMU index of slot (zero based)
 *
 *  Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M704() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_load(val);
}

/**
 * M705: Eject filament from MMU
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  P<mmu>      - MMU index of slot (zero based)
 *
 *  Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M705() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_eject(val);
}

/**
 * M706: Cut filament by MMU
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  P<mmu>      - MMU index of slot (zero based)
 *
 *  Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M706() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_cut(val);
}

__attribute__((weak)) void filament_gcodes::mmu_load(uint8_t data) {}
__attribute__((weak)) void filament_gcodes::mmu_eject(uint8_t data) {}
__attribute__((weak)) void filament_gcodes::mmu_cut(uint8_t data) {}
