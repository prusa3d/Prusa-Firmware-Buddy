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
#include "fs_event_autolock.hpp"

uint filament_gcodes::InProgress::lock = 0;

using namespace filament_gcodes;

/**
 * Shared code for load/unload filament
 */
bool filament_gcodes::load_unload(LoadUnloadMode type, filament_gcodes::Func f_load_unload, pause::Settings &rSettings) {
    float disp_temp = marlin_server_get_temp_to_display();
    float targ_temp = Temperature::degTargetHotend(rSettings.GetExtruder());

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(disp_temp, rSettings.GetExtruder());
    }

    // Load/Unload filament
    bool res = std::invoke(f_load_unload, Pause::Instance(), rSettings);

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(targ_temp, rSettings.GetExtruder());
    }
    return res;
}

void filament_gcodes::M701_no_parser(filament_t filament_to_be_loaded, const std::optional<float> &fast_load_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, int8_t mmu_slot) {
    Filaments::SetToBeLoaded(filament_to_be_loaded);
    if (op_preheat) {
        PreheatData data(!fast_load_length.has_value() || fast_load_length > 0.F ? PreheatMode::Load : PreheatMode::Purge, *op_preheat);
        auto preheat_ret = data.Mode() == PreheatMode::Load ? preheat_for_change_load(data) : preheat(data);
        if (preheat_ret.first) {
            // canceled
            M70X_process_user_response(*preheat_ret.first);
            return;
        }

        filament_t filament = preheat_ret.second;
        Filaments::SetToBeLoaded(filament);
    }

    pause::Settings settings;
    settings.SetExtruder(target_extruder);
    settings.SetFastLoadLength(fast_load_length);
    settings.SetRetractLength(0.f);
    settings.SetMmuFilamentToLoad(mmu_slot);
    xyz_pos_t park_position = { X_AXIS_LOAD_POS, NAN, z_min_pos > 0 ? std::max(current_position.z, z_min_pos) : NAN };
#ifndef DO_NOT_RESTORE_Z_AXIS
    settings.SetResumePoint(current_position);
#endif
    settings.SetParkPoint(park_position);

    load_unload(LoadUnloadMode::Load, &Pause::FilamentLoad, settings);

    M70X_process_user_response(PreheatStatus::Result::DoneHasFilament);
}

void filament_gcodes::M702_no_parser(std::optional<float> unload_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, bool ask_unloaded) {
    if (op_preheat) {
        PreheatData data(PreheatMode::Unload, *op_preheat); // TODO do I need PreheatMode::Unload_askUnloaded
        auto preheat_ret = preheat(data);
        if (preheat_ret.first) {
            //canceled
            M70X_process_user_response(*preheat_ret.first);
            return;
        }
    }

    pause::Settings settings;
    settings.SetUnloadLength(unload_length);
    xyz_pos_t park_position = { X_AXIS_UNLOAD_POS, NAN, z_min_pos > 0 ? std::max(current_position.z, z_min_pos) : NAN };
#ifndef DO_NOT_RESTORE_Z_AXIS
    settings.SetResumePoint(current_position);
#endif
    settings.SetParkPoint(park_position);

    load_unload(LoadUnloadMode::Unload, ask_unloaded ? &Pause::FilamentUnload_AskUnloaded : &Pause::FilamentUnload, settings);

    M70X_process_user_response(PreheatStatus::Result::DoneNoFilament);
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

void SetResult(Result res) {
    preheatResult = res;
}

}

void filament_gcodes::M70X_process_user_response(PreheatStatus::Result res) {
    // modify temperatures
    switch (res) {
    case PreheatStatus::Result::DoneHasFilament:
        thermalManager.setTargetHotend(Filaments::Current().nozzle_preheat, 0);
        break;
    case PreheatStatus::Result::CooledDown:
        // set temperatures to zero
        thermalManager.setTargetHotend(0, 0);
        thermalManager.setTargetBed(0);
        marlin_server_set_temp_to_display(0);
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
    if constexpr (HAS_BOWDEN) {
        Filaments::Set(filament_t::NONE);
        M701_no_parser(filament_t::NONE, fast_load_length, z_min_pos, RetAndCool_t::Return, target_extruder, 0);
    } else {

        pause::Settings settings;
        settings.SetExtruder(target_extruder);
        settings.SetFastLoadLength(fast_load_length);
        settings.SetRetractLength(0.f);

        // catch filament in gear and then ask for temp
        if (!Pause::Instance().LoadToGear(settings)) {
            // do not ask for filament type after stop was pressed
            Pause::Instance().UnloadFromGear();
            M70X_process_user_response(PreheatStatus::Result::DoneNoFilament);
            FSensors_instance().ClrAutoloadSent();
        }

        PreheatData data(PreheatMode::Autoload, RetAndCool_t::Return);
        auto preheat_ret = preheat_for_change_load(data);

        if (preheat_ret.first) {
            // canceled
            Pause::Instance().UnloadFromGear();
            M70X_process_user_response(PreheatStatus::Result::DoneNoFilament);
            FSensors_instance().ClrAutoloadSent();
            return;
        }

        filament_t filament = preheat_ret.second;
        Filaments::SetToBeLoaded(filament);

        load_unload(LoadUnloadMode::Load, &Pause::FilamentAutoload, settings);

        M70X_process_user_response(PreheatStatus::Result::DoneHasFilament);
    }

    FSensors_instance().ClrAutoloadSent();
}

void filament_gcodes::M1600_no_parser(uint8_t target_extruder) {
    FS_EventAutolock autoload_lock;
    InProgress progress;
    filament_t filament = Filaments::CurrentIndex();
    if (filament == filament_t::NONE) {
        PreheatStatus::SetResult(PreheatStatus::Result::DoneNoFilament);
        return;
    } else {
        PreheatStatus::SetResult(PreheatStatus::Result::DoneHasFilament);
    }
    preheat_to(filament);

    pause::Settings settings;
    xyz_pos_t park_position = { X_AXIS_UNLOAD_POS, NAN, std::max(current_position.z, (float)Z_AXIS_LOAD_POS) };
    settings.SetParkPoint(park_position);
    settings.SetExtruder(target_extruder);
    settings.SetRetractLength(0.f);

    load_unload(LoadUnloadMode::Unload, &Pause::FilamentUnload_AskUnloaded, settings);

    // LOAD
    // cannot do normal preheat, since printer is already preheated from unload
    PreheatData data(PreheatMode::Change_phase2, RetAndCool_t::Return);
    auto preheat_ret = preheat_for_change_load(data);
    if (preheat_ret.first) {
        // canceled
        M70X_process_user_response(*preheat_ret.first);
        return;
    }

    filament = preheat_ret.second;
    Filaments::SetToBeLoaded(filament);

    load_unload(LoadUnloadMode::Load, &Pause::FilamentLoad, settings);

    M70X_process_user_response(PreheatStatus::Result::DoneHasFilament);
}
