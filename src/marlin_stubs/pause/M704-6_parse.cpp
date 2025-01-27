/**
 * @file
 */
#include "../PrusaGcodeSuite.hpp"
#include "M70X.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 * @brief Load filament to MMU
 *
 * ## Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 *         For non-mixing, current extruder if omitted.
 * - `P` - MMU index of slot (zero based)
 *
 * Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M704() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_load(val);
}

void PrusaGcodeSuite::M1704() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_load_test(val);
}

/**
 * @brief Eject filament from MMU
 *
 * ## Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 *         For non-mixing, current extruder if omitted.
 * - `P` - MMU index of slot (zero based)
 *
 * Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M705() {
    const uint8_t val = parser.byteval('P', 0);
    filament_gcodes::mmu_eject(val);
}

/**
 * @brief Cut filament by MMU
 *
 * ## Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 *         For non-mixing, current extruder if omitted.
 * - `P` - MMU index of slot (zero based)
 *
 * Default values are used for omitted arguments.
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
