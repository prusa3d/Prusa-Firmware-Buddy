/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../../inc/MarlinConfig.h"

#include <option/has_i2c_expander.h>

#if HAS_I2C_EXPANDER()

#include "../../gcode.h"

#include "../../../feature/twibus.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M260: I2C Send Data <a href="https://reprap.org/wiki/G-code#M260:_i2c_Send_Data">M260: i2c Send Data</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S and XL
 *
 *#### Usage
 *
 *    M260 [ A | B | S | R ]
 *
 *#### Parameters
 *
 * - `A` - Address
 * - `B` - Byte
 * - `S` - Send the buffered data and reset the buffer
 * - `R` - Reset the buffer without sending data
 *
 *  Prusa IO Expander: Only writes first byte of the buffer
 */
void GcodeSuite::M260() {
  // Set the target address
  if (parser.seenval('A')) {
    if (!twibus.address(parser.value_byte())) {
      return;
    }
  }

  // Add a new byte to the buffer
  if (parser.seenval('B')) twibus.addbyte(parser.value_byte());

  // Flush the buffer to the bus
  if (parser.seen('S')) twibus.send();

  // Reset and rewind the buffer
  else if (parser.seen('R')) twibus.reset();
}

/**
 *### M261: I2C Request Data <a href="https://reprap.org/wiki/G-code#M261:_i2c_Request_Data">M261: i2c Request Data</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S and XL
 *
 *#### Usage
 *
 *    M261 [ A | B | S ]
 *
 *#### Parameters
 *
 * - `A` - Address
 * - `B` - Number of Bytes to read
 * - `S` - Output format
 *   - `0` - Character (default)
 *   - `1` - Hexadecimal
 *   - `2` - signed two byte integer
 *   - `3` - unsigned byte, base 10
 *
 * Prusa IO Expander: Only reads one byte and the rest is set to 0
 */
void GcodeSuite::M261() {
  if (parser.seenval('A')) {
    if (!twibus.address(parser.value_byte())) {
      return;
    }
  }

  const uint8_t bytes = parser.byteval('B', 1),   // Bytes to request
                style = parser.byteval('S');      // Serial output style (ASCII, HEX etc)

  if (twibus.addr && bytes && bytes <= TWIBUS_BUFFER_SIZE)
    twibus.relay(bytes, style);
  else
    SERIAL_ERROR_MSG("Bad i2c request");
}

/** @}*/

#endif
