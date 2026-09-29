#include "config_features.h"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"
#include "../../../lib/Marlin/Marlin/src/module/temperature.h"

#include "marlin_server.hpp"
#include <common/marlin_vars.hpp>
#include "pause_stubbed.hpp"
#include <atomic>
#include <functional> // std::invoke
#include <cmath>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include "M70X.hpp"
#include <utils/variant_utils.hpp>
#include <config_store/store_instance.hpp>
#include <filament_to_load.hpp>
#include <Marlin/src/gcode/gcode.h>
#include <mapi/parking.hpp>
#include <raii/auto_restore.hpp>

#include <option/has_human_interactions.h>
#include <option/has_wastebin.h>
#include <option/has_auto_retract.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

#include <option/has_indx.h>
#if HAS_INDX()
    #include <Marlin/src/module/tool_change.h>
    #include <nozzle_cleaner.hpp>
#endif

uint filament_gcodes::InProgress::lock = 0;

using namespace filament_gcodes;

/**
 * Shared code for load/unload filament
 */
static bool load_unload(Pause::LoadType load_type, pause::Settings &rSettings) {
    float disp_temp = marlin_vars().active_hotend().display_nozzle;
    float targ_temp = Temperature::degTargetHotend(rSettings.physical_tool());

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(static_cast<int16_t>(disp_temp), rSettings.physical_tool());
    }

    // Load/Unload filament
    const bool res = Pause::Instance().perform(load_type, rSettings);

    if (marlin_server::printer_idle() && !res) { // Failed when printer is not printing
        // Disable nozzle heater
        thermalManager.setTargetHotend(0, rSettings.physical_tool());
        return false;
    }

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(static_cast<int16_t>(targ_temp), rSettings.physical_tool());
    }
    return res;
}

void filament_gcodes::M701_load(const M701LoadArgs &args) {
    InProgress progress;

    FilamentType filament_to_be_loaded = args.filament_to_be_loaded;
    const VirtualToolIndex virtual_tool = args.virtual_tool;
    const bool do_purge_only = args.fast_load_length.has_value() && *args.fast_load_length <= 0.0f;

    if (args.op_preheat) {
        if (filament_to_be_loaded == FilamentType::none) {
            const FilamentSelectionArgs data {
                .mode = do_purge_only ? PreheatMode::purge : PreheatMode::standard_load,
                .tool = virtual_tool,
                .ret_cool = *args.op_preheat,
#if HAS_ANFC()
                .openprinttag_uid_hash = args.openprinttag_uid_hash,
#endif
            };
            auto preheat_ret = preheat(data, PreheatBehavior::for_filament_load());
            if (preheat_ret.first) {
                // canceled
                M70X_process_user_response(*preheat_ret.first, virtual_tool);
                return;
            }

            filament_to_be_loaded = preheat_ret.second;
        } else {
            preheat_to(filament_to_be_loaded, virtual_tool.to_physical(), PreheatBehavior::for_filament_load(false));
        }
    }

    filament_to_be_loaded = filament::apply_load_color(filament_to_be_loaded, virtual_tool, args.color_to_be_loaded);

    filament::set_type_to_load(filament_to_be_loaded);

    pause::Settings settings;
    settings.SetExtruder(virtual_tool);
    settings.SetFastLoadLength(args.fast_load_length);
    settings.SetRetractLength(0.f);

    mapi::ParkingPosition park_position = mapi::get_parking_position(do_purge_only ? mapi::ParkPosition::purge : mapi::ParkPosition::load);
    park_position.z = mapi::ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE, .absolute = args.z_min_pos };

    settings.SetParkPoint(park_position);
    const xyze_pos_t current_position_tmp = current_position;

    // Pick the right tool
    if (!Pause::Instance().tool_change(virtual_tool, Pause::LoadType::load, settings)) {
        return;
    }

#ifndef DO_NOT_RESTORE_Z_AXIS
    // Has to be set before last Pause operation, otherwise it unparks and parks again inbetween operations
    settings.SetResumePoint(current_position_tmp);
#endif

    const bool do_resume_print = args.resume_print_request && marlin_server::printer_paused();
    // Load
    if (load_unload(do_purge_only ? Pause::LoadType::load_purge : Pause::LoadType::load, settings)) {
        if (!do_resume_print) {
            M70X_process_user_response(PreheatStatus::Result::DoneHasFilament, virtual_tool);
        }
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, virtual_tool);
    }

    // Pretend like we haven't moved the extruder at all
    sync_e_position_to(current_position_tmp.e);
    destination.e = current_position_tmp.e;

#if HAS_INDX()
    // Park the tool back to its dock after load
    if (!do_resume_print) {
        tool_change(NoTool {}, tool_return_t::dock_backoff);
    }
#endif

    if (do_resume_print) {
        marlin_server::print_resume();
    }
}

void filament_gcodes::M702_unload(std::optional<float> unload_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, VirtualToolIndex virtual_tool, bool ask_unloaded) {
    InProgress progress;

    bool do_preheat = op_preheat.has_value();
#if HAS_AUTO_RETRACT()
    do_preheat = do_preheat && !buddy::auto_retract().can_cold_unload(virtual_tool.to_physical());
#endif

    if (do_preheat) {
        const FilamentSelectionArgs data {
            .mode = PreheatMode::unload,
            .tool = virtual_tool,
            .ret_cool = *op_preheat,
        };
        auto preheat_ret = preheat(data, PreheatBehavior::for_filament_unload());
        if (preheat_ret.first) {
            // canceled
            M70X_process_user_response(*preheat_ret.first, virtual_tool);
            return;
        }
    }

    pause::Settings settings;
    settings.SetExtruder(virtual_tool);
    settings.SetUnloadLength(unload_length);
    settings.SetRetractLength(0.f);
    mapi::ParkingPosition park_position = mapi::get_parking_position(mapi::ParkPosition::unload);
    park_position.z = mapi::ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE, .absolute = z_min_pos };
    settings.SetParkPoint(park_position);
    xyze_pos_t current_position_tmp = current_position;

    // Pick the right tool
    if (!Pause::Instance().tool_change(virtual_tool, Pause::LoadType::unload, settings)) {
        return;
    }

#ifndef DO_NOT_RESTORE_Z_AXIS
    // Has to be set before last Pause operation, otherwise it unparks and parks again inbetween operations
    settings.SetResumePoint(current_position_tmp);
#endif

    // Unload
    load_unload(ask_unloaded ? Pause::LoadType::unload_confirm : Pause::LoadType::unload, settings);
    M70X_process_user_response(PreheatStatus::Result::CooledDown, virtual_tool);
    sync_e_position_to(current_position_tmp.e);
    destination.e = current_position_tmp.e;

#if HAS_INDX()
    // Park the tool back to its dock after unload
    tool_change(NoTool {}, tool_return_t::dock_backoff);
#endif
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

void filament_gcodes::M70X_process_user_response(PreheatStatus::Result res, VirtualToolIndex target_extruder) {
    // modify temperatures
    switch (res) {
    case PreheatStatus::Result::DoneHasFilament: {
        const float disp_temp = config_store().get_filament_type(target_extruder).parameters().nozzle_preheat_temperature;
        thermalManager.setTargetHotend(static_cast<int16_t>(disp_temp), target_extruder.to_physical());
        break;
    }
    case PreheatStatus::Result::CooledDown:
        // set temperatures to zero
        thermalManager.setTargetHotend(0, target_extruder.to_physical());
        thermalManager.setTargetBed(0);
        thermalManager.set_print_fan_speed(0);
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

void filament_gcodes::M1600_change_filament(FilamentType filament_to_be_loaded, VirtualToolIndex virtual_tool, RetAndCool_t preheat, AskFilament_t ask_filament, std::optional<Color> color_to_be_loaded) {
    InProgress progress;

    FilamentType filament = config_store().get_filament_type(virtual_tool);
    if (filament == FilamentType::none && ask_filament == AskFilament_t::Never) {
        PreheatStatus::SetResult(PreheatStatus::Result::DoneNoFilament);
        return;
    }

    if (ask_filament == AskFilament_t::Always || (filament == FilamentType::none && ask_filament == AskFilament_t::IfUnknown)) {
        // need to save filament to check if operation went well, PreheatMode::unload for user info in header
        M1700_preheat(M1700Args {
            .preheat = preheat,
            .mode = PreheatMode::unload,
            .tool = virtual_tool,
            .save = true,
            .enforce_target_temp = true,
            .preheat_bed = config_store().filament_change_preheat_all.get(),
#if HAS_CHAMBER_API()
            .preheat_chamber = config_store().filament_change_preheat_all.get(),
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
            .set_heatbreak = true,
#endif
        });
        filament = config_store().get_filament_type(virtual_tool);
        if (filament == FilamentType::none) {
            return; // no need to set PreheatStatus::Result::DoneNoFilament, M1700 did that
        }
    }

    PreheatStatus::SetResult(PreheatStatus::Result::DoneHasFilament);

    bool is_safe_to_unload = false;
#if HAS_AUTO_RETRACT()
    is_safe_to_unload = buddy::auto_retract().can_cold_unload(virtual_tool.to_physical());
#endif

    if (!is_safe_to_unload) {
        preheat_to(filament, virtual_tool.to_physical(), PreheatBehavior::for_filament_unload(false));
    }
    xyze_pos_t current_position_tmp = current_position;

    pause::Settings settings;
    settings.SetParkPoint(mapi::get_parking_position(mapi::ParkPosition::unload));
    settings.SetExtruder(virtual_tool);
    settings.SetRetractLength(0.f);

    // Pick the right tool
    if (!Pause::Instance().tool_change(virtual_tool, Pause::LoadType::unload, settings)) {
        return;
    }

    // Unload
    if (load_unload(PRINTER_IS_PRUSA_iX() ? Pause::LoadType::unload : Pause::LoadType::unload_confirm, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, virtual_tool);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, virtual_tool);
        return;
    }

    // LOAD
    // cannot do normal preheat, since printer is already preheated from unload
    if (filament_to_be_loaded == FilamentType::none) {
        const FilamentSelectionArgs data {
            .mode = PreheatMode::change_load,
            .tool = virtual_tool,
            .ret_cool = preheat,
        };
        auto preheat_ret = ::preheat(data, PreheatBehavior::for_filament_load());
        if (preheat_ret.first) {
            // canceled
            M70X_process_user_response(*preheat_ret.first, virtual_tool);
            return;
        }

        filament_to_be_loaded = preheat_ret.second;
    } else {
        preheat_to(filament_to_be_loaded, virtual_tool.to_physical(), PreheatBehavior::for_filament_load(false));
    }

    filament_to_be_loaded = filament::apply_load_color(filament_to_be_loaded, virtual_tool, color_to_be_loaded);

    filament::set_type_to_load(filament_to_be_loaded);

    // Update park position for load phase (move to front/load position instead of staying at unload/waste bin position)
    settings.SetParkPoint(mapi::get_parking_position(mapi::ParkPosition::load));

#ifndef DO_NOT_RESTORE_Z_AXIS
    // Has to be set before last Pause operation, otherwise it unparks and parks again inbetween operations
    settings.SetResumePoint(current_position_tmp);
#endif

    if (load_unload(Pause::LoadType::load, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneHasFilament, virtual_tool);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, virtual_tool);
    }

#if HAS_INDX()
    // Park the tool back to its dock after change filament
    tool_change(NoTool {}, tool_return_t::dock_backoff);
#endif
}
