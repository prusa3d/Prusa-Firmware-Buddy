#include "config_features.h"

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
#include "filament_sensor_api.hpp"
#include "M70X.hpp"

static Response preheatTempKnown() {
    return Filaments::Current().response;
}

static Response preheatTempUnKnown(PreheatData preheat_data, bool break_on_autoload = false) {
    Response ret;
    FSM_Holder H(ClientFSM::Preheat, preheat_data.Data());
    while ((ret = ClientResponseHandler::GetResponseFromPhase(PhasesPreheat::UserTempSelection)) == Response::_none) {
        if (break_on_autoload && FSensors_instance().IsAutoloadInProgress())
            return Response::_none;
        idle(true, true);
    }
    return ret;
}

static Response evaluate_preheat_conditions(PreheatData preheat_data) {
    Response response = Response::_none;
    bool canKnowTemp = preheat_data.Mode() == PreheatMode::Unload || preheat_data.Mode() == PreheatMode::Change_phase1 || preheat_data.Mode() == PreheatMode::Purge || preheat_data.Mode() == PreheatMode::Unload_askUnloaded;

    // Check if we are using operation which can get temp from printer and check if it can get the temp from available info (inserted filament or set temperature in temperature menu and no filament inserted)
    if (canKnowTemp && ((Filaments::CurrentIndex() != filament_t::NONE))) {
        // We can get temperature without user telling us
        response = preheatTempKnown();
    } else {
        // we need to ask the user for temperature
        response = preheatTempUnKnown(preheat_data);
    }

    return response;
}

std::pair<std::optional<PreheatStatus::Result>, filament_t> filament_gcodes::preheat(PreheatData preheat_data) {

    Response response = evaluate_preheat_conditions(preheat_data);

    filament_t filament = Filaments::Find(response);

    // No filament selected or selected cooldown when it is possible
    if (filament == filament_t::NONE) {
        switch (response) {
        case Response::Abort:
            return { PreheatStatus::Result::Aborted, filament_t::NONE };
        case Response::Cooldown:
            return { PreheatStatus::Result::CooledDown, filament_t::NONE };
        default: // should not happen
            return { PreheatStatus::Result::Error, filament_t::NONE };
        }
    }

    preheat_to(filament);
    return { std::nullopt, filament };
}

void filament_gcodes::preheat_to(filament_t filament) {

    const Filament &fil_cnf = Filaments::Get(filament);

    // change temp only if it is lower than currently loaded filament
    if (thermalManager.degTargetHotend(0) < fil_cnf.nozzle) {
        thermalManager.setTargetHotend(fil_cnf.nozzle, 0);
        marlin_server_set_temp_to_display(fil_cnf.nozzle);
        thermalManager.setTargetBed(fil_cnf.heatbed);
    }
}

std::pair<std::optional<PreheatStatus::Result>, filament_t> filament_gcodes::preheat_for_change_load(PreheatData data) {

    Response response = preheatTempUnKnown(data);

    filament_t filament = Filaments::Find(response);

    // No filament selected or selected cooldown when it is possible
    if (filament == filament_t::NONE) {
        switch (response) {
        case Response::Abort:
            return { PreheatStatus::Result::Aborted, filament_t::NONE };
        case Response::Cooldown:
            return { PreheatStatus::Result::CooledDown, filament_t::NONE };
        default: // should not happen
            return { PreheatStatus::Result::Error, filament_t::NONE };
        }
    }

    const Filament &fil_cnf = Filaments::Get(filament);

    // change temp every time (unlike normal preheat)
    thermalManager.setTargetHotend(fil_cnf.nozzle, 0);
    marlin_server_set_temp_to_display(fil_cnf.nozzle);
    thermalManager.setTargetBed(fil_cnf.heatbed);

    return { std::nullopt, filament };
}

/**
 * @brief stand alone preheat
 *
 * @param preheat_tp preheat options
 * @param target_extruder
 */
void filament_gcodes::M1700_no_parser(RetAndCool_t preheat_tp, uint8_t target_extruder, bool save, bool enforce_target_temp) {
    PreheatData data(PreheatMode::None, preheat_tp);
    Response response = preheatTempUnKnown(data, true);

    // autoload ocurred
    if (response == Response::_none) {
        return;
    }

    filament_t filament = Filaments::Find(response);

    if (response != Response::Abort) {
        const Filament &fil_cnf = Filaments::Get(filament);

        thermalManager.setTargetHotend(enforce_target_temp ? fil_cnf.nozzle : fil_cnf.nozzle_preheat, 0);
        marlin_server_set_temp_to_display(fil_cnf.nozzle);
        thermalManager.setTargetBed(fil_cnf.heatbed);
        //cooldown pressed
        if (filament == filament_t::NONE) {
            thermalManager.set_fan_speed(0, 0);
        }
    }

    if (save)
        Filaments::Set(filament);

    // store result, so other threads can see it
    PreheatStatus::SetResult(PreheatStatus::Result::DoneNoFilament);

    // we might want to set filament type even with preheat, if so do:
    //Filaments::SetToBeLoaded(filament);
}
