#include "filament_sensors_handler.hpp"
#include <config_store/store_instance.hpp>

// clang-format off
#if (!ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)) || \
    HAS_LCD_MENU || \
    ENABLED(MIXING_EXTRUDER) || \
    ENABLED(NO_MOTION_BEFORE_HOMING)
    #error unsupported
#endif
// clang-format on

#include "../../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"
#include "../../../lib/Marlin/Marlin/src/module/temperature.h"
#include "pause_stubbed.hpp"
#include "filament_sensors_handler.hpp"
#include "M70X.hpp"

static Response preheatTempKnown(uint8_t target_extruder) {
    auto filament_type = config_store().get_filament_type(target_extruder);
    assert(filament_type != filament::Type::NONE);
    return filament::get_description(filament_type).response;
}

static Response preheatTempUnKnown(PreheatData preheat_data, bool break_on_autoload = false) {
    Response ret;
    marlin_server::FSM_Holder holder { PhasesPreheat::UserTempSelection, preheat_data.serialize() };
    while ((ret = marlin_server::get_response_from_phase(PhasesPreheat::UserTempSelection)) == Response::_none) {
        if (preheat_data.Mode() == PreheatMode::Autoload && FSensors_instance().sensor_state(LogicalFilamentSensor::autoload) == FilamentSensorState::NoFilament) {
            return Response::Abort;
        }
        if (break_on_autoload && FSensors_instance().IsAutoloadInProgress()) {
            return Response::_none;
        }
        idle(true, true);
    }
    return ret;
}

static Response evaluate_preheat_conditions(PreheatData preheat_data, uint8_t target_extruder) {
    Response response = Response::_none;
    bool canKnowTemp = preheat_data.Mode() == PreheatMode::Unload || preheat_data.Mode() == PreheatMode::Change_phase1 || preheat_data.Mode() == PreheatMode::Purge || preheat_data.Mode() == PreheatMode::Unload_askUnloaded;

    // Check if we are using operation which can get temp from printer and check if it can get the temp from available info (inserted filament or set temperature in temperature menu and no filament inserted)
    if (canKnowTemp && ((config_store().get_filament_type(target_extruder) != filament::Type::NONE))) {
        // We can get temperature without user telling us
        response = preheatTempKnown(target_extruder);
    } else {
        // we need to ask the user for temperature
        response = preheatTempUnKnown(preheat_data);
    }

    return response;
}

std::pair<std::optional<PreheatStatus::Result>, filament::Type> filament_gcodes::preheat(PreheatData preheat_data, uint8_t target_extruder) {

    Response response = evaluate_preheat_conditions(preheat_data, target_extruder);

    filament::Type filament = filament::get_type(response);

    // No filament selected or selected cooldown when it is possible
    if (filament == filament::Type::NONE) {
        switch (response) {
        case Response::Abort:
            return { PreheatStatus::Result::Aborted, filament::Type::NONE };
        case Response::Cooldown:
            return { PreheatStatus::Result::CooledDown, filament::Type::NONE };
        default: // should not happen
            return { PreheatStatus::Result::Error, filament::Type::NONE };
        }
    }

    preheat_to(filament, target_extruder);
    return { std::nullopt, filament };
}

void filament_gcodes::preheat_to(filament::Type filament, uint8_t target_extruder) {

    const filament::Description &fil_cnf = filament::get_description(filament);

    // change temp only if it is lower than currently loaded filament
    if (thermalManager.degTargetHotend(target_extruder) < fil_cnf.nozzle) {
        thermalManager.setTargetHotend(fil_cnf.nozzle, target_extruder);
        marlin_server::set_temp_to_display(fil_cnf.nozzle, target_extruder);
        if (config_store().heatup_bed.get()) {
            thermalManager.setTargetBed(fil_cnf.heatbed);
        }
    }
}

std::pair<std::optional<PreheatStatus::Result>, filament::Type> filament_gcodes::preheat_for_change_load(PreheatData data, uint8_t target_extruder) {

    Response response = preheatTempUnKnown(data);

    filament::Type filament = filament::get_type(response);

    // No filament selected or selected cooldown when it is possible
    if (filament == filament::Type::NONE) {
        switch (response) {
        case Response::Abort:
            return { PreheatStatus::Result::Aborted, filament::Type::NONE };
        case Response::Cooldown:
            return { PreheatStatus::Result::CooledDown, filament::Type::NONE };
        default: // should not happen
            return { PreheatStatus::Result::Error, filament::Type::NONE };
        }
    }

    const filament::Description &fil_cnf = filament::get_description(filament);

    // change temp every time (unlike normal preheat)
    thermalManager.setTargetHotend(fil_cnf.nozzle, target_extruder);
    marlin_server::set_temp_to_display(fil_cnf.nozzle, target_extruder);
    if (config_store().heatup_bed.get()) {
        thermalManager.setTargetBed(fil_cnf.heatbed);
    }

    return { std::nullopt, filament };
}

void filament_gcodes::M1700_no_parser(RetAndCool_t preheat_tp, PreheatMode mode, int8_t target_extruder, bool save, bool enforce_target_temp, bool preheat_bed) {
    InProgress progress;
    PreheatData data(mode, preheat_tp);
    Response response = preheatTempUnKnown(data, true);

    // autoload ocurred
    if (response == Response::_none) {
        return;
    }

    filament::Type filament = filament::get_type(response);

    if (response != Response::Abort) {
        const filament::Description &fil_cnf = filament::get_description(filament);

        if (response == Response::Cooldown // Cooldown is always applied to all tools
            || target_extruder < 0) {
            // Set temperature to all tools
            HOTEND_LOOP() {
#if ENABLED(PRUSA_TOOLCHANGER)
                if (!prusa_toolchanger.is_tool_enabled(e)) {
                    continue;
                }
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/
                thermalManager.setTargetHotend(enforce_target_temp ? fil_cnf.nozzle : fil_cnf.nozzle_preheat, e);
                marlin_server::set_temp_to_display(fil_cnf.nozzle, e);
            }
        } else {
            // Preheat only target tool
            thermalManager.setTargetHotend(enforce_target_temp ? fil_cnf.nozzle : fil_cnf.nozzle_preheat, target_extruder);
            marlin_server::set_temp_to_display(fil_cnf.nozzle, target_extruder);
        }

        if (preheat_bed) {
            thermalManager.setTargetBed(fil_cnf.heatbed);
        }

        // cooldown pressed
        if (filament == filament::Type::NONE) {
            thermalManager.set_fan_speed(0, 0);
        } else if ((axis_homed & _BV(Z_AXIS)) != _BV(Z_AXIS)) {
            unhomed_z_lift(10);
        }

        if (save) {
            if (target_extruder < 0) {
                HOTEND_LOOP() {
                    config_store().set_filament_type(e, filament);
                }
            } else {
                config_store().set_filament_type(target_extruder, filament);
            }
        }
    }

    // store result, so other threads can see it
    PreheatStatus::SetResult(response != Response::Abort ? PreheatStatus::Result::DoneNoFilament : PreheatStatus::Result::Aborted);

    // we might want to set filament type even with preheat, if so do:
    // Filaments::SetToBeLoaded(filament);
}
