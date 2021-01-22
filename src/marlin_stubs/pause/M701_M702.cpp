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

#include "../../../lib/Marlin/Marlin/src/inc/MarlinConfigPre.h"

// clang-format off
#if (!ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)) || \
    EXTRUDERS > 1 || \
    HAS_LCD_MENU || \
    ENABLED(PRUSA_MMU2) || \
    ENABLED(MIXING_EXTRUDER) || \
    ENABLED(NO_MOTION_BEFORE_HOMING)
    #error unsupported
#endif
// clang-format on

#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/temperature.h"
#include "../PrusaGcodeSuite.hpp"
#include "marlin_server.hpp"
#include "pause_stubbed.hpp"
#include "filament.hpp"
#include <functional> // std::invoke
#include <algorithm>
#include <cmath>

#define DO_NOT_RESTORE_Z_AXIS
static const constexpr uint8_t Z_AXIS_LOAD_POS = 40;
static const constexpr uint8_t Z_AXIS_UNLOAD_POS = 20;

using Func = bool (Pause::*)(); //member fnc pointer

/**
 * Shared code for load/unload filament
 */
static void load_unload(LoadUnloadMode type, Func f_load_unload, uint32_t min_Z_pos) {
    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    float disp_temp = marlin_server_get_temp_to_display();
    float targ_temp = Temperature::degTargetHotend(target_extruder);

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(disp_temp, target_extruder);
    }
    // Z axis lift
    if (parser.seenval('Z'))
        min_Z_pos = parser.linearval('Z');

    xyz_pos_t park_position = current_position;
    if (min_Z_pos > 0) {
        static constexpr float Z_max = Z_MAX_POS;
        park_position.z = std::min(std::max(current_position.z, float(min_Z_pos)), Z_max);
    }
#ifdef DO_NOT_RESTORE_Z_AXIS
    xyze_pos_t resume_position = park_position;
#else
    xyze_pos_t resume_position = current_position;
#endif
    Pause &pause = Pause::Instance();
    pause.SetParkPoint(park_position);
    pause.SetResumePoint(resume_position);

    // Load/Unload filament
    std::invoke(f_load_unload, pause);

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(targ_temp, target_extruder);
    }
}

/**
 * M701: Load filament
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  Z<distance> - Move the Z axis by this distance
 *  L<distance> - Extrude distance for insertion (positive value) (manual reload)
 *  S"Filament" - save filament by name, for example S"PLA". RepRap compatible.
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M701() {
    Filaments::SetToBeLoaded(Filaments::Default);
    const char *text_begin = 0;
    if (parser.seen('S')) {
        text_begin = strchr(parser.string_arg, '"');
        if (text_begin) {
            ++text_begin; //move pointer from '"' to first letter
            const char *text_end = strchr(text_begin, '"');
            if (text_end) {
                filament_t filament = Filaments::FindByName(text_begin, text_end - text_begin);
                if (filament != filament_t::NONE) {
                    Filaments::SetToBeLoaded(filament);
                }
            }
        }
    }
    Pause &pause = Pause::Instance();
    const bool isL = (parser.seen('L') && (!text_begin || strchr(parser.string_arg, 'L') < text_begin));
    const float fast_load_length = std::abs(isL ? parser.value_axis_units(E_AXIS) : pause.GetDefaultFastLoadLength());
    pause.SetPurgeLength(ADVANCED_PAUSE_PURGE_LENGTH);
    pause.SetSlowLoadLength(fast_load_length > 0.f ? FILAMENT_CHANGE_SLOW_LOAD_LENGTH : 0.f);
    pause.SetFastLoadLength(fast_load_length);
    pause.SetRetractLength(0.f);

    load_unload(fast_load_length != 0.f ? LoadUnloadMode::Load : LoadUnloadMode::Purge, &Pause::FilamentLoad, Z_AXIS_LOAD_POS);
}

/**
 * M702: Unload filament
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, if omitted, current extruder
 *                (or ALL extruders with FILAMENT_UNLOAD_ALL_EXTRUDERS).
 *  Z<distance> - Move the Z axis by this distance
 *  U<distance> - Retract distance for removal (manual reload)
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M702() {
    Pause::Instance().SetUnloadLength(parser.seen('U') ? parser.value_axis_units(E_AXIS) : NAN);
    load_unload(
        LoadUnloadMode::Unload, &Pause::FilamentUnload, Z_AXIS_UNLOAD_POS);
}

/**
 * M1400: Preheat
 * not meant to be used during print
 *
 *  S<bit fields value> - [0 - 1] type - 0 NONE
 *                                     - 1 LOAD
 *                                     - 2 UNLOAD
 *                                     - 3 change
 *                      - [2 - 5] reserved
 *                      - [6] has return option
 *                      - [7] has cooldown option
 *                      - [8 - 31] reserved
 *
 *  Default value S0
 */
void PrusaGcodeSuite::M1400() {
    const uint32_t val = parser.ulongval('S', 0);
    const PreheatMode mode = PreheatMode(val & 0x03);

    Response ret;
    // preheat part
    {
        FSM_Holder H(ClientFSM::Preheat, uint8_t(mode));
        while ((ret = ClientResponseHandler::GetResponseFromPhase(PhasesPreheat::UserTempSelection)) == Response::_none) {
            idle(true);
        }
    }

    filament_t filament = Filaments::Find(ret);

    /*

    case Response::ABS:
    case Response::ASA:
    case Response::FLEX:
    case Response::HIPS:
    case Response::PC:
    case Response::PETG:
    case Response::PLA:
    case Response::PP:
*/

    if (filament == filament_t::NONE) {
        switch (ret) {
        case Response::Abort:
            return;
        case Response::Cooldown:
            //set temperatures to zero
            thermalManager.setTargetHotend(0, 0);
            thermalManager.setTargetBed(0);
            marlin_server_set_temp_to_display(0);
            return;
        default: //should not happen
            return;
        }
    }

    const Filament &fil_cnf = Filaments::Get(filament);
    thermalManager.setTargetHotend(fil_cnf.nozzle, 0);
    thermalManager.setTargetBed(fil_cnf.heatbed);
    marlin_server_set_temp_to_display(fil_cnf.nozzle);

    switch (mode) {
    case PreheatMode::None:
        break;
    case PreheatMode::Load:
        break;
    case PreheatMode::Unload:
        break;
    case PreheatMode::Change:
        break;
    }
}
