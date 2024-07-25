#include "config_features.h"
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

static FSMResponseVariant preheatTempKnown(uint8_t target_extruder) {
    const FilamentType filament_type = config_store().get_filament_type(target_extruder);
    assert(filament_type != FilamentType::none);
    return FSMResponseVariant::make(filament_type);
}

static FSMResponseVariant preheatTempUnKnown(PreheatData preheat_data, bool break_on_autoload = false) {
    marlin_server::FSM_Holder holder { PhasesPreheat::UserTempSelection, preheat_data.serialize() };

    while (true) {
        if (const auto ret = marlin_server::get_response_variant_from_phase(PhasesPreheat::UserTempSelection)) {
            return ret;
        }
        if (preheat_data.Mode() == PreheatMode::Autoload && FSensors_instance().sensor_state(LogicalFilamentSensor::autoload) == FilamentSensorState::NoFilament) {
            return FSMResponseVariant::make(Response::Abort);
        }
        if (break_on_autoload && FSensors_instance().IsAutoloadInProgress()) {
            return FSMResponseVariant();
        }

        idle(true, true);
    }
}

static FSMResponseVariant evaluate_preheat_conditions(PreheatData preheat_data, uint8_t target_extruder) {
    bool canKnowTemp = preheat_data.Mode() == PreheatMode::Unload || preheat_data.Mode() == PreheatMode::Change_phase1 || preheat_data.Mode() == PreheatMode::Purge || preheat_data.Mode() == PreheatMode::Unload_askUnloaded;

    // Check if we are using operation which can get temp from printer and check if it can get the temp from available info (inserted filament or set temperature in temperature menu and no filament inserted)
    if (canKnowTemp && ((config_store().get_filament_type(target_extruder) != FilamentType::none))) {
        // We can get temperature without user telling us
        return preheatTempKnown(target_extruder);

    } else {
        // we need to ask the user for temperature
        return preheatTempUnKnown(preheat_data);
    }
}

std::pair<std::optional<PreheatStatus::Result>, FilamentType> filament_gcodes::preheat(PreheatData preheat_data, uint8_t target_extruder) {
    const FSMResponseVariant response = evaluate_preheat_conditions(preheat_data, target_extruder);

    if (response.holds_alternative<FilamentType>()) {
        const FilamentType filament = response.value<FilamentType>();
        preheat_to(filament, target_extruder);
        return { std::nullopt, filament };
    }

    switch (response.value_or(Response::_none)) {

    case Response::Abort:
        return { PreheatStatus::Result::Aborted, FilamentType::none };

    case Response::Cooldown:
        return { PreheatStatus::Result::CooledDown, FilamentType::none };

    default:
        // should not happen
        return { PreheatStatus::Result::Error, FilamentType::none };
    }
}

void filament_gcodes::preheat_to(FilamentType filament, uint8_t target_extruder) {
    const FilamentTypeParameters fil_cnf = filament.parameters();

    // change temp only if it is lower than currently loaded filament
    if (thermalManager.degTargetHotend(target_extruder) < fil_cnf.nozzle_temperature) {
        thermalManager.setTargetHotend(fil_cnf.nozzle_temperature, target_extruder);
        marlin_server::set_temp_to_display(fil_cnf.nozzle_temperature, target_extruder);
        if (config_store().heatup_bed.get()) {
            thermalManager.setTargetBed(fil_cnf.heatbed_temperature);
        }
    }
}

std::pair<std::optional<PreheatStatus::Result>, FilamentType> filament_gcodes::preheat_for_change_load(PreheatData data, uint8_t target_extruder) {
    const FSMResponseVariant response = preheatTempUnKnown(data);

    if (response.holds_alternative<FilamentType>()) {
        const FilamentType filament = response.value<FilamentType>();
        const FilamentTypeParameters fil_cnf = filament.parameters();

        // change temp every time (unlike normal preheat)
        thermalManager.setTargetHotend(fil_cnf.nozzle_temperature, target_extruder);
        marlin_server::set_temp_to_display(fil_cnf.nozzle_temperature, target_extruder);
        if (config_store().heatup_bed.get()) {
            thermalManager.setTargetBed(fil_cnf.heatbed_temperature);
        }

        return { std::nullopt, filament };
    }

    switch (response.value_or(Response::_none)) {

    case Response::Abort:
        return { PreheatStatus::Result::Aborted, FilamentType::none };

    case Response::Cooldown:
        return { PreheatStatus::Result::CooledDown, FilamentType::none };

    default:
        // should not happen
        return { PreheatStatus::Result::Error, FilamentType::none };
    }
}

void filament_gcodes::M1700_no_parser(RetAndCool_t preheat_tp, PreheatMode mode, int8_t target_extruder, bool save, bool enforce_target_temp, bool preheat_bed) {
    InProgress progress;
    const FSMResponseVariant response_variant = preheatTempUnKnown(PreheatData(mode, preheat_tp), true);

    // autoload ocurred
    if (!response_variant) {
        return;
    }

    const Response response = response_variant.value_or(Response::_none);
    if (response == Response::Abort) {
        PreheatStatus::SetResult(PreheatStatus::Result::Aborted);
        return;
    }

    const FilamentType filament = response_variant.value_or<FilamentType>(FilamentType::none);
    const FilamentTypeParameters fil_cnf = filament.parameters();

    const auto set_extruder_temp = [&](uint8_t extruder) {
        thermalManager.setTargetHotend(enforce_target_temp ? fil_cnf.nozzle_temperature : fil_cnf.nozzle_preheat_temperature, extruder);
        marlin_server::set_temp_to_display(fil_cnf.nozzle_temperature, extruder);
    };

    if (response == Response::Cooldown || target_extruder < 0) {
        // Set temperature to all tools
        // Cooldown is always applied to all tools
        HOTEND_LOOP() {
#if ENABLED(PRUSA_TOOLCHANGER)
            if (!prusa_toolchanger.is_tool_enabled(e)) {
                continue;
            }
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/
            set_extruder_temp(e);
        }

    } else {
        // Preheat only target tool
        set_extruder_temp(target_extruder);
    }

    if (preheat_bed) {
        thermalManager.setTargetBed(fil_cnf.heatbed_temperature);
    }

    // cooldown pressed
    if (filament == FilamentType::none) {
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

    // store result, so other threads can see it
    PreheatStatus::SetResult(PreheatStatus::Result::DoneNoFilament);

    // we might want to set filament type even with preheat, if so do:
    // Filaments::SetToBeLoaded(filament);
}
