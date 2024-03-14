/// @file
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "PrusaGcodeSuite.hpp"
#include <device/peripherals.h>
#include <option/has_gui.h>
#include "main.h"

/**
 * @brief Set up the prescaler of the LCD perifery SPI.
 *
 * This is used for manipulating communication frequency during HW testing.
 * If value was not set or was set 0, prescaler will set up 0 (frequency will be divided by 1 = unchanged).
 *
 *   [uint16]       Prescaler value (0-7) is mapped internally on power of 2
 */

void PrusaGcodeSuite::M930() {
#if BOARD_IS_BUDDY && HAS_GUI()
    uint16_t val = parser.value_ushort();
    spi_set_prescaler(&SPI_HANDLE_FOR(lcd), val);
#endif
}

/**
 * @brief Set up the prescaler of the EXT_FLASH perifery SPI.
 *
 * This is used for manipulating communication frequency during HW testing.
 * If value was not set or was set 0, prescaler will set up 0 (frequency will be divided by 1 = unchanged).
 *
 *   [uint16]       Prescaler value (0-7) is mapped internally on power of 2
 */

void PrusaGcodeSuite::M931() {
#if BOARD_IS_BUDDY
    uint16_t val = parser.value_ushort();
    spi_set_prescaler(&SPI_HANDLE_FOR(flash), val);
#endif
}

/**
 * @brief Set up the prescaler of the TMC perifery SPI.
 *
 * This is used for manipulating communication frequency during HW testing.
 * If value was not set or was set 0, prescaler will set up 0 (frequency will be divided by 1 = unchanged).
 *
 *   [uint16]       Prescaler value (0-7) is mapped internally on power of 2
 */

void PrusaGcodeSuite::M932() {
#if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
    uint16_t val = parser.value_ushort();
    spi_set_prescaler(&SPI_HANDLE_FOR(tmc), val);
#endif
}
