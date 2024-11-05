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
#include <atomic>
#include <functional> // std::invoke
#include <cmath>
#include "filament_sensors_handler.hpp"
#include "M70X.hpp"
#include <config_store/store_instance.hpp>
#include <filament_to_load.hpp>
#include <Marlin/src/gcode/gcode.h>
#include <marlin_stubs/pause/G27.hpp>

#include <option/has_bowden.h>
#include <option/has_human_interactions.h>
#include <option/has_wastebin.h>

uint filament_gcodes::InProgress::lock = 0;

using namespace filament_gcodes;

/**
 * Shared code for load/unload filament
 */
bool filament_gcodes::load_unload([[maybe_unused]] LoadUnloadMode type, filament_gcodes::Func f_load_unload, pause::Settings &rSettings) {
    float disp_temp = marlin_vars().active_hotend().display_nozzle;
    float targ_temp = Temperature::degTargetHotend(rSettings.GetExtruder());

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(disp_temp, rSettings.GetExtruder());
    }

    // Load/Unload filament
    bool res = std::invoke(f_load_unload, Pause::Instance(), rSettings);

    if (marlin_server::printer_idle() && !res) { // Failed when printer is not printing
        // Disable nozzle heater
        thermalManager.setTargetHotend(0, rSettings.GetExtruder());
        marlin_server::set_temp_to_display(0, rSettings.GetExtruder());
        return false;
    }

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(targ_temp, rSettings.GetExtruder());
    }
    return res;
}

void filament_gcodes::M701_no_parser(FilamentType filament_to_be_loaded, const std::optional<float> &fast_load_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, int8_t mmu_slot, std::optional<Color> color_to_be_loaded, ResumePrint_t resume_print_request) {
    InProgress progress;

    marlin_server::DisableNozzleTimeout disableNozzleTimeout;
    if (marlin_server::printer_paused()) {
        marlin_server::unpause_nozzle(target_extruder);
    }

    const bool do_purge_only = fast_load_length.has_value() && fast_load_length <= 0.0f;

    if (op_preheat) {
        if (filament_to_be_loaded == FilamentType::none) {
            PreheatData data = PreheatData::make(do_purge_only ? PreheatMode::Purge : PreheatMode::Load, target_extruder, *op_preheat);
            auto preheat_ret = data.mode == PreheatMode::Load ? preheat_for_change_load(data, target_extruder) : preheat(data, target_extruder);
            if (preheat_ret.first) {
                // canceled
                M70X_process_user_response(*preheat_ret.first, target_extruder);
                return;
            }

            filament_to_be_loaded = preheat_ret.second;
        } else {
            preheat_to(filament_to_be_loaded, target_extruder);
        }
    }
    filament::set_type_to_load(filament_to_be_loaded);
    filament::set_color_to_load(color_to_be_loaded);

    pause::Settings settings;
    settings.SetExtruder(target_extruder);
    settings.SetFastLoadLength(fast_load_length);
    settings.SetRetractLength(0.f);
    settings.SetMmuFilamentToLoad(mmu_slot);

    xyz_pos_t park_position = { X_AXIS_LOAD_POS, Y_AXIS_LOAD_POS, z_min_pos > 0 ? std::max(current_position.z, z_min_pos) : NAN };

    settings.SetParkPoint(park_position);
    xyze_pos_t current_position_tmp = current_position;

    // Pick the right tool
    if (!Pause::Instance().ToolChange(target_extruder, LoadUnloadMode::Load, settings)) {
        return;
    }

#ifndef DO_NOT_RESTORE_Z_AXIS
    // Has to be set before last Pause operation, otherwise it unparks and parks again inbetween operations
    settings.SetResumePoint(current_position_tmp);
#endif

    const bool do_resume_print = static_cast<bool>(resume_print_request) && marlin_server::printer_paused();
    // Load
    if (load_unload(LoadUnloadMode::Load, option::has_human_interactions ? (do_purge_only ? &Pause::FilamentPurge : &Pause::FilamentLoad) : &Pause::FilamentLoadNotBlocking, settings)) {
        if (!do_resume_print) {
            M70X_process_user_response(PreheatStatus::Result::DoneHasFilament, target_extruder);
        }
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
    }
    planner.set_e_position_mm((destination.e = current_position.e = current_position_tmp.e));

    if (do_resume_print) {
        marlin_server::print_resume();
    }
}

void filament_gcodes::M702_no_parser(std::optional<float> unload_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, bool ask_unloaded) {
    InProgress progress;

    marlin_server::DisableNozzleTimeout disableNozzleTimeout;
    if (marlin_server::printer_paused()) {
        marlin_server::unpause_nozzle(target_extruder);
    }

    if (op_preheat) {
        PreheatData data = PreheatData::make(PreheatMode::Unload, target_extruder, *op_preheat); // TODO do I need PreheatMode::Unload_askUnloaded
        auto preheat_ret = preheat(data, target_extruder);
        if (preheat_ret.first) {
            // canceled
            M70X_process_user_response(*preheat_ret.first, target_extruder);
            return;
        }
    }

    pause::Settings settings;
    settings.SetExtruder(target_extruder);
    settings.SetUnloadLength(unload_length);
    settings.SetRetractLength(0.f);
    xyz_pos_t park_position = { X_AXIS_UNLOAD_POS, Y_AXIS_UNLOAD_POS, z_min_pos > 0 ? std::max(current_position.z, z_min_pos) : NAN };
    settings.SetParkPoint(park_position);
    xyze_pos_t current_position_tmp = current_position;

    // Pick the right tool
    if (!Pause::Instance().ToolChange(target_extruder, LoadUnloadMode::Unload, settings)) {
        return;
    }

#ifndef DO_NOT_RESTORE_Z_AXIS
    // Has to be set before last Pause operation, otherwise it unparks and parks again inbetween operations
    settings.SetResumePoint(current_position_tmp);
#endif

    // Unload
    if (load_unload(LoadUnloadMode::Unload, ask_unloaded ? &Pause::FilamentUnload_AskUnloaded : &Pause::FilamentUnload, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, target_extruder);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
    }
    planner.set_e_position_mm((destination.e = current_position.e = current_position_tmp.e));
}

namespace PreheatStatus {

static std::atomic<Result> preheatResult = Result::DidNotFinish;

Result ConsumeResult() {
    return preheatResult.exchange(Result::DidNotFinish);
}

void SetResult(Result res) {
    preheatResult.store(res);
}

} // namespace PreheatStatus

void filament_gcodes::M70X_process_user_response(PreheatStatus::Result res, uint8_t target_extruder) {
    // modify temperatures
    switch (res) {
    case PreheatStatus::Result::DoneHasFilament: {
        const float disp_temp = config_store().get_filament_type(target_extruder).parameters().nozzle_preheat_temperature;
        thermalManager.setTargetHotend(disp_temp, target_extruder);
        marlin_server::set_temp_to_display(disp_temp, target_extruder);
        break;
    }
    case PreheatStatus::Result::CooledDown:
        // set temperatures to zero
        thermalManager.setTargetHotend(0, target_extruder);
        thermalManager.setTargetBed(0);
        marlin_server::set_temp_to_display(0, target_extruder);
        thermalManager.set_fan_speed(0, 0);
        break;
    case PreheatStatus::Result::DoneNoFilament:
    case PreheatStatus::Result::Aborted:
    case PreheatStatus::Result::Error:
    case PreheatStatus::Result::DidNotFinish: // cannot happen
    default:
        break; // do not alter temp
    }

    // store result, so other threads can see it
    PreheatStatus::SetResult(res);
}

void filament_gcodes::M1701_no_parser(const std::optional<float> &fast_load_length, float z_min_pos, uint8_t target_extruder) {
    filament::set_type_to_load(FilamentType::none);
    filament::set_color_to_load(std::nullopt);

    InProgress progress;
    if constexpr (option::has_bowden) {
        config_store().set_filament_type(target_extruder, FilamentType::none);
        M701_no_parser(FilamentType::none, fast_load_length, z_min_pos, RetAndCool_t::Return, target_extruder, 0, std::nullopt, ResumePrint_t::No);
    } else {

        pause::Settings settings;
        settings.SetExtruder(target_extruder);
        settings.SetFastLoadLength(fast_load_length);
        settings.SetRetractLength(0.f);
        float e_pos_to_restore = current_position.e;

        settings.SetParkPoint({ X_AXIS_LOAD_POS, Y_AXIS_LOAD_POS, z_min_pos > 0 ? std::max(current_position.z, z_min_pos) : NAN });

        // catch filament in gear and then ask for temp
        if (!Pause::Instance().LoadToGear(settings) || FSensors_instance().no_filament_surely()) {
            // do not ask for filament type after stop was pressed or filament was removed from FS
            Pause::Instance().UnloadFromGear();
            M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, target_extruder);
            FSensors_instance().ClrAutoloadSent();
            return;
        }

        if constexpr (option::has_human_interactions) {
            PreheatData data = PreheatData::make(PreheatMode::Autoload, target_extruder, RetAndCool_t::Return);
            auto preheat_ret = preheat_for_change_load(data, target_extruder);

            if (preheat_ret.first) {
                // canceled
                Pause::Instance().UnloadFromGear();
                M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, target_extruder);
                FSensors_instance().ClrAutoloadSent();
                return;
            }

            const FilamentType filament = preheat_ret.second;
            filament::set_type_to_load(filament);
            filament::set_color_to_load(std::nullopt);

            if (z_min_pos > 0 && z_min_pos > current_position.z + 0.1F) {
                xyz_pos_t park_position = { NAN, NAN, z_min_pos };
                // Returning to previous position is unwanted outside of printing (M1701 should be used only outside of printing)
                settings.SetParkPoint(park_position);
            }

            if (load_unload(LoadUnloadMode::Load, &Pause::FilamentAutoload, settings)) {
                M70X_process_user_response(PreheatStatus::Result::DoneHasFilament, target_extruder);
            } else {
                M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
            }
        }
        planner.set_e_position_mm((destination.e = current_position.e = e_pos_to_restore));
    }

    FSensors_instance().ClrAutoloadSent();
}

void filament_gcodes::M1600_no_parser(FilamentType filament_to_be_loaded, uint8_t target_extruder, RetAndCool_t preheat, AskFilament_t ask_filament, std::optional<Color> color_to_be_loaded) {
    InProgress progress;

    FilamentType filament = config_store().get_filament_type(target_extruder);
    if (filament == FilamentType::none && ask_filament == AskFilament_t::Never) {
        PreheatStatus::SetResult(PreheatStatus::Result::DoneNoFilament);
        return;
    }

    if (ask_filament == AskFilament_t::Always || (filament == FilamentType::none && ask_filament == AskFilament_t::IfUnknown)) {
        // need to save filament to check if operation went well, PreheatMode::Unload for user info in header
        M1700_no_parser(preheat, PreheatMode::Unload, target_extruder, true, true, config_store().heatup_bed.get());
        filament = config_store().get_filament_type(target_extruder);
        if (filament == FilamentType::none) {
            return; // no need to set PreheatStatus::Result::DoneNoFilament, M1700 did that
        }
    }

    PreheatStatus::SetResult(PreheatStatus::Result::DoneHasFilament);

    preheat_to(filament, target_extruder);
    xyze_pos_t current_position_tmp = current_position;

    pause::Settings settings;
    xyz_pos_t park_position = { X_AXIS_UNLOAD_POS, Y_AXIS_UNLOAD_POS, std::max(current_position.z, (float)Z_AXIS_LOAD_POS) };
    settings.SetParkPoint(park_position);
    settings.SetExtruder(target_extruder);
    settings.SetRetractLength(0.f);

    // Pick the right tool
    if (!Pause::Instance().ToolChange(target_extruder, LoadUnloadMode::Unload, settings)) {
        return;
    }

    // Unload
    if (load_unload(LoadUnloadMode::Unload, PRINTER_IS_PRUSA_iX() ? &Pause::FilamentUnload : &Pause::FilamentUnload_AskUnloaded, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, target_extruder);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
        return;
    }

    // LOAD
    // cannot do normal preheat, since printer is already preheated from unload
    if (filament_to_be_loaded == FilamentType::none) {
        PreheatData data = PreheatData::make(PreheatMode::Change_phase2, target_extruder, preheat);
        auto preheat_ret = preheat_for_change_load(data, target_extruder);
        if (preheat_ret.first) {
            // canceled
            M70X_process_user_response(*preheat_ret.first, target_extruder);
            return;
        }

        filament_to_be_loaded = preheat_ret.second;
    } else {
        preheat_to(filament_to_be_loaded, target_extruder);
    }
    filament::set_type_to_load(filament_to_be_loaded);
    filament::set_color_to_load(color_to_be_loaded);

#ifndef DO_NOT_RESTORE_Z_AXIS
    // Has to be set before last Pause operation, otherwise it unparks and parks again inbetween operations
    settings.SetResumePoint(current_position_tmp);
#endif

    if (load_unload(LoadUnloadMode::Load, PRINTER_IS_PRUSA_iX() ? &Pause::FilamentLoadNotBlocking : &Pause::FilamentLoad, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneHasFilament, target_extruder);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
    }
}
