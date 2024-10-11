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
#include <option/has_modularbed.h>
#include <option/has_dwarf.h>
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>

#if HAS_TOOLCHANGER()
    #include "../../module/prusa/toolchanger.h"
#endif

#if ENABLED(EXTENDED_CAPABILITIES_REPORT)
static void cap_line(PGM_P const name, bool ena = false) {
    SERIAL_ECHOPGM("Cap:");
    serialprintPGM(name);
    SERIAL_CHAR(':');
    SERIAL_ECHOLN(int(ena ? 1 : 0));
}
#endif

#include <version/version.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M115: Capabilities string <a href="https://reprap.org/wiki/G-code#M115:_Get_Firmware_Version_and_Capabilities">M115: Get Firmware Version and Capabilities</a>
 *
 *#### Usage
 *
 *    M115
 *
 */
void GcodeSuite::M115() {
    SERIAL_ECHOPGM("FIRMWARE_NAME:Prusa-Firmware-Buddy ");
    SERIAL_ECHOPGM(version::project_version_full);
    SERIAL_ECHOPGM(" (Github) SOURCE_CODE_URL:https://github.com/prusa3d/Prusa-Firmware-Buddy");
    SERIAL_ECHOPGM(" PROTOCOL_VERSION:" PROTOCOL_VERSION " MACHINE_TYPE:Prusa-" PRINTER_MODEL);

    uint8_t extruder_count = 0;
#if HAS_MMU2()
    // Devices with MMU2/MMU3 only have one physical extruder, whereas EXTRUDERS refers to total amount of virtual extruders
    // As only physical extruders are reported here, we hardcode the value to 1
    extruder_count = 1;
#elif HAS_TOOLCHANGER()
    extruder_count = prusa_toolchanger.get_num_enabled_tools();
#else
    extruder_count = EXTRUDERS;
#endif
    SERIAL_ECHOPGM(" EXTRUDER_COUNT:");
    SERIAL_ECHO(extruder_count);

    SERIAL_ECHOLNPGM(" UUID:" MACHINE_UUID);
#if ENABLED(EXTENDED_CAPABILITIES_REPORT)

    // SERIAL_XON_XOFF
    cap_line(PSTR("SERIAL_XON_XOFF")
    #if ENABLED(SERIAL_XON_XOFF)
                 ,
        true
    #endif
    );

    // BINARY_FILE_TRANSFER (M28 B1)
    cap_line(PSTR("BINARY_FILE_TRANSFER")
    #if ENABLED(BINARY_FILE_TRANSFER)
                 ,
        true
    #endif
    );

    // EEPROM (M500, M501)
    cap_line(PSTR("EEPROM")
    #if ENABLED(EEPROM_SETTINGS)
                 ,
        true
    #endif
    );

    // Volumetric Extrusion (M200)
    cap_line(PSTR("VOLUMETRIC")
    #if DISABLED(NO_VOLUMETRICS)
                 ,
        true
    #endif
    );

    // AUTOREPORT_TEMP (M155)
    cap_line(PSTR("AUTOREPORT_TEMP")
    #if ENABLED(AUTO_REPORT_TEMPERATURES)
                 ,
        true
    #endif
    );

    // PROGRESS (M530 S L, M531 <file>, M532 X L)
    cap_line(PSTR("PROGRESS"));

    // Print Job timer M75, M76, M77
    cap_line(PSTR("PRINT_JOB"), true);

    // AUTOLEVEL (G29)
    cap_line(PSTR("AUTOLEVEL")
    #if HAS_AUTOLEVEL
                 ,
        true
    #endif
    );

    // Z_PROBE (G30)
    cap_line(PSTR("Z_PROBE")
    #if HAS_BED_PROBE
                 ,
        true
    #endif
    );

    // MESH_REPORT (M420 V)
    cap_line(PSTR("LEVELING_DATA")
    #if HAS_LEVELING
                 ,
        true
    #endif
    );

    // BUILD_PERCENT (M73)
    cap_line(PSTR("BUILD_PERCENT")
    #if ENABLED(LCD_SET_PROGRESS_MANUALLY)
                 ,
        true
    #endif
    );

    // SOFTWARE_POWER (M80, M81)
    cap_line(PSTR("SOFTWARE_POWER")
    #if HAS_POWER_SWITCH
                 ,
        true
    #endif
    );

    // CASE LIGHTS (M355)
    cap_line(PSTR("TOGGLE_LIGHTS")
    #if HAS_CASE_LIGHT
                 ,
        true
    #endif
    );
    cap_line(PSTR("CASE_LIGHT_BRIGHTNESS")
    #if HAS_CASE_LIGHT
                 ,
        PWM_PIN(CASE_LIGHT_PIN)
    #endif
    );

    // EMERGENCY_PARSER (M108, M112, M410, M876)
    cap_line(PSTR("EMERGENCY_PARSER")
    #if ENABLED(EMERGENCY_PARSER)
                 ,
        true
    #endif
    );

    // PROMPT SUPPORT (M876)
    cap_line(PSTR("PROMPT_SUPPORT")
    #if ENABLED(HOST_PROMPT_SUPPORT)
                 ,
        true
    #endif
    );

    // AUTOREPORT_SD_STATUS (M27 extension)
    cap_line(PSTR("AUTOREPORT_SD_STATUS")
    #if ENABLED(AUTO_REPORT_SD_STATUS)
                 ,
        true
    #endif
    );

    // THERMAL_PROTECTION
    cap_line(PSTR("THERMAL_PROTECTION")
    #if ((ENABLED(THERMAL_PROTECTION_HOTENDS) || HAS_DWARF()) && (ENABLED(THERMAL_PROTECTION_BED) || !HAS_HEATED_BED || HAS_MODULARBED()) && (ENABLED(THERMAL_PROTECTION_CHAMBER) || !HAS_HEATED_CHAMBER))
                 ,
        true
    #endif
    );

    // MOTION_MODES (M80-M89)
    cap_line(PSTR("MOTION_MODES")
    #if ENABLED(GCODE_MOTION_MODES)
                 ,
        true
    #endif
    );

    // CHAMBER_TEMPERATURE (M141, M191)
    cap_line(PSTR("CHAMBER_TEMPERATURE")
    #if HAS_HEATED_CHAMBER
                 ,
        true
    #endif
    );

#endif // EXTENDED_CAPABILITIES_REPORT
}

/** @}*/
