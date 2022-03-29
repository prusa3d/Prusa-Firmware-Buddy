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
#if (!ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)) || \
    HAS_LCD_MENU || \
    ENABLED(MIXING_EXTRUDER) || \
    ENABLED(NO_MOTION_BEFORE_HOMING)
    #error unsupported
#endif
// clang-format on

#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"
#include "../../../lib/Marlin/Marlin/src/module/temperature.h"

#include "marlin_server.hpp"
#include "pause_stubbed.hpp"
#include <functional> // std::invoke
#include <cmath>
#include "task.h" //critical sections
#include "filament_sensor_api.hpp"
#include "eeprom_function_api.h"
#include "RAII.hpp"
#include "M70X.hpp"

using namespace m1400;
static Pause &pause = Pause::Instance();

/**
 * Shared code for load/unload filament
 */
bool m1400::load_unload(LoadUnloadMode type, m1400::Func f_load_unload, uint32_t min_Z_pos, float X_pos) {
    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return false;

    float disp_temp = marlin_server_get_temp_to_display();
    float targ_temp = Temperature::degTargetHotend(target_extruder);

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(disp_temp, target_extruder);
    }

    xyz_pos_t park_position = current_position;
    if (min_Z_pos > 0) {
        static const float Z_max = get_z_max_pos_mm();
        park_position.z = std::min(std::max(current_position.z, float(min_Z_pos)), Z_max);
    }

    // This is there to move the nozzle further away from side of printer to ease the strain on the PTFE tube while loading or unloading
    if (!isnan(X_pos)) {
        park_position.x = std::clamp(float(X_pos), float(X_MIN_POS), float(X_MAX_POS));
    }

#ifdef DO_NOT_RESTORE_Z_AXIS
    xyze_pos_t resume_position = park_position;
#else
    xyze_pos_t resume_position = current_position;
#endif
    pause.SetParkPoint(park_position);
    pause.SetResumePoint(resume_position);

    // Load/Unload filament
    bool res = std::invoke(f_load_unload, pause);

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(targ_temp, target_extruder);
    }
    return res;
}

void m1400::M701_no_parser(filament_t filament_to_be_loaded, float fast_load_length, float z_min_pos) {
    Filaments::SetToBeLoaded(filament_to_be_loaded);
    pause.SetPurgeLength(ADVANCED_PAUSE_PURGE_LENGTH);
    pause.SetSlowLoadLength(fast_load_length > 0.f ? FILAMENT_CHANGE_SLOW_LOAD_LENGTH : 0.f);
    pause.SetFastLoadLength(fast_load_length);
    pause.SetRetractLength(0.f);

    m1400::load_unload(fast_load_length != 0.f ? LoadUnloadMode::Load : LoadUnloadMode::Purge, &Pause::FilamentLoad, z_min_pos, X_AXIS_LOAD_POS);
}

static PreheatStatus::Result M1400_NoParser_LoadUnload(PreheatData data, uint8_t mmu_command_data = 0) {
    auto preheat_ret = preheat(data);
    if (preheat_ret.first)
        return *preheat_ret.first;
    filament_t filament = preheat_ret.second;

    switch (data.Mode()) {
    case PreheatMode::None:
        return Filaments::CurrentIndex() == filament_t::NONE ? PreheatStatus::Result::DoneNoFilament : PreheatStatus::Result::DoneHasFilament;
    case PreheatMode::Purge:
        Pause::Instance().StopReset();
        M701_no_parser(filament, 0);
        return PreheatStatus::Result::DoneHasFilament;
    case PreheatMode::Autoload:
        // bowden extruder does normal load instead autoload
        // because it does not have filament loaded in gear
        if constexpr (HAS_BOWDEN) {
            Filaments::SetToBeLoaded(filament);
            pause.SetPurgeLength(ADVANCED_PAUSE_PURGE_LENGTH);
            pause.SetSlowLoadLength(FILAMENT_CHANGE_SLOW_LOAD_LENGTH);
            pause.SetFastLoadLength(pause.GetDefaultFastLoadLength());
            pause.SetRetractLength(0.f);
            load_unload(LoadUnloadMode::Load, &Pause::FilamentAutoload, Z_AXIS_UNLOAD_POS, X_AXIS_UNLOAD_POS);
            return PreheatStatus::Result::DoneHasFilament;
        } // HAS_BOWDEN
        //continue to load for !HAS_BOWDEN
    case PreheatMode::Load:
        Pause::Instance().StopReset();
    case PreheatMode::Change_phase2:
        M701_no_parser(filament, pause.GetDefaultFastLoadLength());
        return PreheatStatus::Result::DoneHasFilament;
    case PreheatMode::MMU_load:
        Pause::Instance().StopReset();
        Filaments::SetToBeLoaded(filament);
        pause.SetPurgeLength(ADVANCED_PAUSE_PURGE_LENGTH);
        pause.SetSlowLoadLength(FILAMENT_CHANGE_SLOW_LOAD_LENGTH);
        pause.SetFastLoadLength(NAN);
        pause.SetRetractLength(0.f);
        pause.SetMmuFilamentToLoad(mmu_command_data);
        m1400::load_unload(LoadUnloadMode::Load, &Pause::FilamentLoad_MMU, Z_AXIS_LOAD_POS, X_AXIS_LOAD_POS);
        return PreheatStatus::Result::DoneHasFilament;
    case PreheatMode::Unload:
        Pause::Instance().SetUnloadLength(NAN);
        Pause::Instance().StopReset();
        load_unload(LoadUnloadMode::Unload, &Pause::FilamentUnload, Z_AXIS_UNLOAD_POS, X_AXIS_UNLOAD_POS);
        return PreheatStatus::Result::DoneNoFilament;
    case PreheatMode::MMU_unload:
        Pause::Instance().SetUnloadLength(NAN);
        Pause::Instance().StopReset();
        m1400::load_unload(LoadUnloadMode::Unload, &Pause::FilamentUnload_MMU, Z_AXIS_UNLOAD_POS, X_AXIS_UNLOAD_POS);
        return PreheatStatus::Result::DoneNoFilament;
        break;
    case PreheatMode::Change_phase1:
        Pause::Instance().SetUnloadLength(NAN);
        Pause::Instance().StopReset();
        if (!load_unload(LoadUnloadMode::Unload, &Pause::FilamentUnload, Z_AXIS_UNLOAD_POS, X_AXIS_UNLOAD_POS))
            return PreheatStatus::Result::Error;
        if (data.Mode() == PreheatMode::Change_phase1) {
            // 2nd preheat, recursion
            return M1400_NoParser_LoadUnload(PreheatData(data.Mode()).Data() /*this cuts of return and cooldown options*/ | uint32_t(PreheatMode::Change_phase2));
        } else {
            return PreheatStatus::Result::DoneNoFilament;
        }
    case PreheatMode::Unload_askUnloaded:
        Pause::Instance().SetUnloadLength(NAN);
        load_unload(LoadUnloadMode::Unload, &Pause::FilamentUnload_AskUnloaded, Z_AXIS_UNLOAD_POS, X_AXIS_UNLOAD_POS);
        return PreheatStatus::Result::DoneNoFilament;
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

__attribute__((weak)) PreheatStatus::Result process_mmu_command(mmu_command_t cmd, uint8_t data) {
    return PreheatStatus::Result::DoneNoFilament; // TODO better response
}

void m1400::M1400_no_parser(PreheatData data, mmu_command_t mmu_cmd, uint8_t mmu_command_data) {
    PreheatStatus::Result res = mmu_cmd == mmu_command_t::no_command ? M1400_NoParser_LoadUnload(data, mmu_command_data)
                                                                     : process_mmu_command(mmu_cmd, mmu_command_data);

    // modify temperatures
    switch (res) {
    case PreheatStatus::Result::DoneHasFilament:
        thermalManager.setTargetHotend(Filaments::PreheatTemp, 0);
        break;
    case PreheatStatus::Result::CooledDown:
        // set temperatures to zero
        thermalManager.setTargetHotend(0, 0);
        thermalManager.setTargetBed(0);
        marlin_server_set_temp_to_display(0);
        break;
    case PreheatStatus::Result::DoneNoFilament:
    case PreheatStatus::Result::Aborted:
    case PreheatStatus::Result::Error:
    case PreheatStatus::Result::DidNotFinish: // cannot happen
    default:
        break; // do not alter temp
    }

    // store result, so other threads can see it
    PreheatStatus::setResult(res);

    FSensors_instance().ClrAutoloadSent();
}
