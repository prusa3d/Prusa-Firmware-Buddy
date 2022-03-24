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
#include "M70X.hpp"

static Pause &pause = Pause::Instance();

static Response preheatTempKnown() {
    return Filaments::Current().response;
}

static Response preheatTempUnKnown(PreheatData data) {
    Response ret;
    FSM_Holder H(ClientFSM::Preheat, data.Data());
    while ((ret = ClientResponseHandler::GetResponseFromPhase(PhasesPreheat::UserTempSelection)) == Response::_none) {
        idle(true, true);
    }
    return ret;
}

static Response evaluate_preheat_conditions(PreheatData data) {
    // check if we are executing operation which can know temperature from the printer state
    bool tempKnownWithoutUser = false;
    switch (data.Mode()) {
    case PreheatMode::MMU_command:
        return Response::_none;
    case PreheatMode::MMU_unload:
    case PreheatMode::Unload:
    case PreheatMode::Purge:
    case PreheatMode::Change_phase1:
    case PreheatMode::Unload_askUnloaded:
        tempKnownWithoutUser = true;
    default:
        break;
    }

    // catch filament in gear and then ask for temp
    if (data.Mode() == PreheatMode::Autoload) {
        pause.SetParkPoint(current_position);
        pause.SetSlowLoadLength(FILAMENT_CHANGE_SLOW_LOAD_LENGTH);
        if (!pause.LoadToGear()) {
            //do not ask for filament type after stop was pressed
            return Response::Abort;
        }
    }

    Response response = Response::_none;

    // Check if we are using operation which can get temp from printer and check if it can get the temp from available info (inserted filament or set temperature in temperature menu and no filament inserted)
    if (tempKnownWithoutUser && ((Filaments::CurrentIndex() != filament_t::NONE) || (Filaments::CurrentIndex() == filament_t::NONE && !thermalManager.targetTooColdToExtrude(0)))) {
        // We can get temperature without user telling us
        response = preheatTempKnown();
    } else {
        // we need to ask the user for temperature
        response = preheatTempUnKnown(data);
    }

    return response;
}

std::pair<std::optional<PreheatStatus::Result>, filament_t> m1400::preheat(PreheatData data) {

    Response response = evaluate_preheat_conditions(data);

    filament_t filament = Filaments::Find(response);

    // No filament selected or selected cooldown when it is possible
    if (filament == filament_t::NONE) {
        switch (response) {
        case Response::Abort:
            // If we have aborted autoload we need to eject filament from the gears
            if (data.Mode() == PreheatMode::Autoload) {
                pause.UnloadFromGear();
            }
            return { PreheatStatus::Result::Aborted, filament_t::NONE };
        case Response::Cooldown:
            return { PreheatStatus::Result::CooledDown, filament_t::NONE };
        default: // should not happen
            return { PreheatStatus::Result::Error, filament_t::NONE };
        }
    }

    const Filament &fil_cnf = Filaments::Get(filament);

    // change temp only if it is lower than currently loaded filament
    if (thermalManager.degTargetHotend(0) < fil_cnf.nozzle) {
        thermalManager.setTargetHotend(fil_cnf.nozzle, 0);
        marlin_server_set_temp_to_display(fil_cnf.nozzle);
        thermalManager.setTargetBed(fil_cnf.heatbed);
    }
    return { std::nullopt, filament };
}
