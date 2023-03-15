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

#include "config_features.h"

// clang-format off
#if (!ENABLED(ADVANCED_PAUSE_FEATURE)) || \
    HAS_LCD_MENU || \
    ENABLED(MMU2_MENUS) || \
    ENABLED(MIXING_EXTRUDER) || \
    ENABLED(DUAL_X_CARRIAGE) || \
    HAS_BUZZER
    #error unsupported
#endif
// clang-format on

#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/temperature.h"
#include "marlin_server.hpp"
#include "pause_stubbed.hpp"
#include <cmath>
#include "filament_sensors_handler.hpp"
#include "filament.hpp"
#if HAS_LEDS
    #include "led_animations/printer_animation_state.hpp"
#endif

/**
 * M600: Pause for filament change
 *
 *  E[distance] - Retract the filament this far
 *  Z[distance] - Move the Z axis by this distance
 *  X[position] - Move to this X position, with Y
 *  Y[position] - Move to this Y position, with X
 *  U[distance] - Retract distance for removal (manual reload)
 *  L[distance] - Extrude distance for insertion (manual reload)
 *  B[count]    - Number of times to beep, -1 for indefinite (if equipped with a buzzer)
 *  T[toolhead] - Select extruder for filament change
 *
 *  Default values are used for omitted arguments.
 */

void GcodeSuite::M600() {
    const int8_t target_extruder = get_target_extruder_from_command();
    if (target_extruder < 0)
        return;
#if HAS_LEDS
    auto guard = PrinterStateAnimation::force_printer_state(PrinterState::Warning);
#endif

    xyz_pos_t park_point =
#ifdef NOZZLE_PARK_POINT_M600
        NOZZLE_PARK_POINT_M600;
#else
        NOZZLE_PARK_POINT;
#endif

    // Lift Z axis
    if (parser.seenval('Z'))
        park_point.z = parser.linearval('Z');

    // Move XY axes to filament change position or given position
    if (parser.seenval('X'))
        park_point.x = parser.linearval('X');
    if (parser.seenval('Y'))
        park_point.y = parser.linearval('Y');

#if HAS_HOTEND_OFFSET && NONE(DUAL_X_CARRIAGE, DELTA) && DISABLED(PRUSA_TOOLCHANGER)
    park_point += hotend_offset[active_extruder];
#endif

    park_point.z += current_position.z;
    static const xyze_float_t no_return = { NAN, NAN, NAN, current_position.e };

    pause::Settings settings;
    settings.SetParkPoint(park_point);
    settings.SetResumePoint(parser.seen('N') ? no_return : current_position);
    if (parser.seen('U'))
        settings.SetUnloadLength(parser.value_axis_units(E_AXIS));
    if (parser.seen('L'))
        settings.SetFastLoadLength(parser.value_axis_units(E_AXIS));
    if (parser.seen('E')) {
        settings.SetRetractLength(std::abs(parser.value_axis_units(E_AXIS)));
    } // Initial retract before move to filament change position

    float disp_temp = marlin_vars()->hotend(target_extruder).display_nozzle;
    float targ_temp = Temperature::degTargetHotend(target_extruder);

    marlin_server_nozzle_timeout_off();
    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(disp_temp, target_extruder);
    }

    filament::set_type_to_load(filament::get_type_in_extruder(target_extruder));
    Pause::Instance().FilamentChange(settings);
    FSensors_instance().ClrM600Sent(); //reset filament sensor M600 sent flag

    marlin_server_nozzle_timeout_on();
    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(targ_temp, target_extruder);
    }
}
