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

#include "../../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"

// clang-format off
#if (!ENABLED(ADVANCED_PAUSE_FEATURE)) || \
    EXTRUDERS > 1 || \
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

/**
 * M602: Unpause / Resume print
 */

void GcodeSuite::M602() {
    const int8_t target_extruder = get_target_extruder_from_command();

    float disp_temp = marlin_server_get_temp_to_display();
    float targ_temp = Temperature::degTargetHotend(target_extruder);

    pause.PrintResume();

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(targ_temp, target_extruder);
    }
}
