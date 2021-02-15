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
#include "task.h" //critical sections
#include "preheat_multithread_status.hpp"
#include "fs_event_autolock.hpp"

#define DO_NOT_RESTORE_Z_AXIS
static const constexpr uint8_t Z_AXIS_LOAD_POS = 40;
static const constexpr uint8_t Z_AXIS_UNLOAD_POS = 20;

using Func = bool (Pause::*)(); //member fnc pointer
static Pause &pause = Pause::Instance();

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
    pause.SetParkPoint(park_position);
    pause.SetResumePoint(resume_position);

    // Load/Unload filament
    std::invoke(f_load_unload, pause);

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(targ_temp, target_extruder);
    }
}

void M701_no_parser(filament_t filament_to_be_laded, float fast_load_length) {
    Filaments::SetToBeLoaded(filament_to_be_laded);
    pause.SetPurgeLength(ADVANCED_PAUSE_PURGE_LENGTH);
    pause.SetSlowLoadLength(fast_load_length > 0.f ? FILAMENT_CHANGE_SLOW_LOAD_LENGTH : 0.f);
    pause.SetFastLoadLength(fast_load_length);
    pause.SetRetractLength(0.f);

    load_unload(fast_load_length != 0.f ? LoadUnloadMode::Load : LoadUnloadMode::Purge, &Pause::FilamentLoad, Z_AXIS_LOAD_POS);
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
    filament_t filament_to_be_laded = Filaments::Default;
    const char *text_begin = 0;
    if (parser.seen('S')) {
        text_begin = strchr(parser.string_arg, '"');
        if (text_begin) {
            ++text_begin; //move pointer from '"' to first letter
            const char *text_end = strchr(text_begin, '"');
            if (text_end) {
                filament_t filament = Filaments::FindByName(text_begin, text_end - text_begin);
                if (filament != filament_t::NONE) {
                    filament_to_be_laded = filament;
                }
            }
        }
    }
    const bool isL = (parser.seen('L') && (!text_begin || strchr(parser.string_arg, 'L') < text_begin));
    const float fast_load_length = std::abs(isL ? parser.value_axis_units(E_AXIS) : pause.GetDefaultFastLoadLength());
    M701_no_parser(filament_to_be_laded, fast_load_length);
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

static PreheatStatus::Result M1400_no_parser(uint32_t val) {
    const PreheatData data(val);

    Response ret;
    // preheat part
    if ((data.Mode() == PreheatMode::Unload || data.Mode() == PreheatMode::Change_phase1) && Filaments::CurrentIndex() != filament_t::NONE) {
        //do not preheat
        ret = Filaments::Current().response; //fake response
    } else {
        FSM_Holder H(ClientFSM::Preheat, uint8_t(val)); //this must remain inside scope
        while ((ret = ClientResponseHandler::GetResponseFromPhase(PhasesPreheat::UserTempSelection)) == Response::_none) {
            idle(true);
        }
    }

    filament_t filament = Filaments::Find(ret);

    if (filament == filament_t::NONE) {
        switch (ret) {
        case Response::Abort:
            return PreheatStatus::Result::Aborted;
        case Response::Cooldown:
            return PreheatStatus::Result::CooledDown;
        default: //should not happen
            return PreheatStatus::Result::Error;
        }
    }

    // filament != filament_t::NONE
    const Filament &fil_cnf = Filaments::Get(filament);
    thermalManager.setTargetHotend(fil_cnf.nozzle, 0);
    thermalManager.setTargetBed(fil_cnf.heatbed);
    marlin_server_set_temp_to_display(fil_cnf.nozzle);

    switch (data.Mode()) {
    case PreheatMode::None:
        return Filaments::CurrentIndex() == filament_t::NONE ? PreheatStatus::Result::DoneNoFilament : PreheatStatus::Result::DoneHasFilament;
    case PreheatMode::Purge:
        M701_no_parser(filament, 0);
        return PreheatStatus::Result::DoneHasFilament;
    case PreheatMode::Load:
    case PreheatMode::Change_phase2:
        M701_no_parser(filament, pause.GetDefaultFastLoadLength());
        return PreheatStatus::Result::DoneHasFilament;
    case PreheatMode::Unload:
    case PreheatMode::Change_phase1:
        Pause::Instance().SetUnloadLength(NAN);
        load_unload(LoadUnloadMode::Unload, &Pause::FilamentUnload, Z_AXIS_UNLOAD_POS);

        if (data.Mode() == PreheatMode::Change_phase1) {
            //2nd preheat, recursion
            return M1400_no_parser((val & ~0x07) | uint32_t(PreheatMode::Change_phase2));
        } else {
            return PreheatStatus::Result::DoneNoFilament;
        }
    default:
        return PreheatStatus::Result::Error;
    }
    return PreheatStatus::Result::Error;
}

namespace PreheatStatus {

volatile static Result preheatResult = Result::DidNotFinish;

Result ConsumeResult() {
    taskENTER_CRITICAL();
    Result ret = preheatResult;
    preheatResult = Result::DidNotFinish;
    taskEXIT_CRITICAL();
    return ret;
}

static void setResult(Result res) {
    preheatResult = res;
}

}

/**
 * M1400: Preheat
 * not meant to be used during print
 *
 *  S<bit fields value> - [0 - 2] PreheatMode - 0 None
 *                                            - 1 Load
 *                                            - 2 Unload
 *                                            - 3 Purge
 *                                            - 4 Change_phase1 == unload + recursively call Change_phase2
 *                                            - 5 Change_phase2 (internal use only, do load)
 *                      - [3 - 5] reserved
 *                      - [6] has return option
 *                      - [7] has cooldown option, PreheatMode must be PreheatMode::None, othervise ignored
 *                      - [8 - 31] reserved
 *
 *  Default value S0
 */
void PrusaGcodeSuite::M1400() {
    FS_EventAutolock LOCK;
    const uint32_t val = parser.ulongval('S', 0);
    PreheatStatus::Result res = M1400_no_parser(val);

    // modify temperatures
    switch (res) {
    case PreheatStatus::Result::DoneHasFilament:
        thermalManager.setTargetHotend(Filaments::PreheatTemp, 0);
        break;
    case PreheatStatus::Result::CooledDown:
        //set temperatures to zero
        thermalManager.setTargetHotend(0, 0);
        thermalManager.setTargetBed(0);
        marlin_server_set_temp_to_display(0);
        break;
    case PreheatStatus::Result::DoneNoFilament:
    case PreheatStatus::Result::Aborted:
    case PreheatStatus::Result::Error:
    case PreheatStatus::Result::DidNotFinish: //cannot happen
    default:
        break; //do not alter temp
    }

    // store result, so other threads can see it
    PreheatStatus::setResult(res);

    FS_instance().ClrAutoloadSent();
}
