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

#include <inc/MarlinConfig.h>

#if ENABLED(CANCEL_OBJECTS)

    #include <src/gcode/gcode.h>
    #include <feature/cancel_object.h>

    #include <marlin_vars.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 * M486: A simple interface to cancel objects
 *
 *   T[count] : Reset objects and/or set the count
 *   S<index> : Start an object with the given index
 *   P<index> : Cancel the object with the given index
 *   U<index> : Un-cancel object with the given index
 *   C        : Cancel the current object (the last index given by S<index>)
 *   S-1      : Start a non-object like a brim or purge tower that should always print
 *
 *   Aname    : Name the current object
 *   Nname    : Legacy, same as Aname
 *   A and N need to be alone in the G-code line, use "M486 S1\nM486 AMyAwesomeObject".
 *   Spaces in name can get consumed by meatpack.
 */
void GcodeSuite::M486() {

    ///@note This must be before other checks and return when A or N is found,
    ///    otherwise objects with C in name get themselves canceled immediately.
    char *arg = nullptr;
    size_t len = strlen(parser.command_ptr);
    if (len >= 6 && (parser.command_ptr[4] == 'A' || parser.command_ptr[4] == 'N')) { // Catch "M486Aname" and "M486Nname"
        arg = parser.command_ptr + 5;
        len -= 5;
    } else if (len >= 7 && parser.command_ptr[4] == ' ' && (parser.command_ptr[5] == 'A' || parser.command_ptr[5] == 'N')) { // Catch "M486 Aname" and "M486 Nname"
        arg = parser.command_ptr + 6;
        len -= 6;
    } else if (parser.boolval('A') || parser.boolval('N')) {
        return; // Ignore the entire line if A or N are not at the first position
    }

    if (arg) {
        if (static_cast<size_t>(cancelable.active_object) >= marlin_vars_t::CANCEL_OBJECTS_NAME_COUNT) {
            return; // out of bounds, nothing can be done
        }
        if (*arg == '\"') {
            arg++; // Skip "
            len--;
        }
        if (len > 0) {
            if (arg[len - 1] == '\"') {
                len--; // Remove " at the end
            }
            marlin_vars().cancel_object_names[cancelable.active_object].set(arg, len);
        }
        return; // Do not parse the line if A or N are found
    }

    if (parser.seen('T')) {
        cancelable.reset();
        cancelable.object_count = parser.intval('T', 1);
    }

    if (parser.seenval('S')) {
        cancelable.set_active_object(parser.value_int());
    }

    if (parser.seen('C')) {
        cancelable.cancel_active_object();
    }

    if (parser.seenval('P')) {
        cancelable.cancel_object(parser.value_int());
    }

    if (parser.seenval('U')) {
        cancelable.uncancel_object(parser.value_int());
    }
}

/** @}*/

#endif // CANCEL_OBJECTS
