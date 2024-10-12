/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../gcode.h"
#include "../../module/configuration_store.h"
#include "../../core/serial.h"
#include "../../inc/MarlinConfig.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M500: Store settings in EEPROM <a href="https://reprap.org/wiki/G-code#M500:_Store_parameters_in_non-volatile_storage">M500: Store parameters in non-volatile storage</a>
 *
 * Save all configurable settings to EEPROM.
 *
 *#### Usage
 *
 *    M500
 */
void GcodeSuite::M500() {
  (void)settings.save();
}

/**
 *### M501: Read settings from EEPROM <a href="https://reprap.org/wiki/G-code#M501:_Read_parameters_from_EEPROM">M501: Read parameters from EEPROM</a>
 *
 * Load all saved settings from EEPROM.
 *
 *#### Usage
 *
 *    M501
 */
void GcodeSuite::M501() {
  (void)settings.load();
}

/**
 *### M502: Revert to default settings <a href="https://reprap.org/wiki/G-code#M502:_Restore_Default_Settings">M502: Restore Default Settings</a>
 *
 * Reset all configurable settings to their factory defaults. This only changes the settings in memory, not on EEPROM.
 *
 *#### Usage
 *
 *    M502
 */
void GcodeSuite::M502() {
  (void)settings.reset();
}

#if DISABLED(DISABLE_M503)

 /**
 *### M503: print settings currently in memory <a href="https://reprap.org/wiki/G-code#M503:_Report_Current_Settings">M503: Report Current Settings</a>
 *
 * Print a concise report of all runtime-configurable settings (in SRAM) to the host console.
 * This command reports the active settings which may or may not be the same as those stored in the EEPROM.
 *
 *#### Usage
 *
 *    M503
 */
  void GcodeSuite::M503() {
    (void)settings.report(!parser.boolval('S', true));
  }

#endif // !DISABLE_M503

/** @}*/

#if ENABLED(EEPROM_SETTINGS)
 /**
 *### M504: Validate EEPROM Contents <a href="https://reprap.org/wiki/G-code#M504:_Validate_EEPROM">M504: Validate EEPROM</a>
 *
 * Validate the contents of the EEPROM.
 *
 *#### Usage
 *
 *    M504
 */
  void GcodeSuite::M504() {
    if (settings.validate())
      SERIAL_ECHO_MSG("EEPROM OK");
  }
#endif
