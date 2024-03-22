/**
 * @file MItem_preheat.cpp
 */

#include "MItem_preheat.hpp"
#include "sound.hpp"
#include "DialogHandler.hpp"
#include "marlin_client.hpp"
#include "ScreenHandler.hpp"
#include <config_store/store_instance.hpp>

/*****************************************************************************/
// MI_PREHEAT
MI_PREHEAT::MI_PREHEAT()
    : MI_event_dispatcher(_(label)) {}

void MI_PREHEAT::Do() {
    marlin_client::gcode_printf("M1700 T-1");
}

/*****************************************************************************/
// MI_PREHEAT_NOZZLE
MI_PREHEAT_NOZZLE::MI_PREHEAT_NOZZLE()
    : MI_event_dispatcher(_(label)) {}

void MI_PREHEAT_NOZZLE::Do() {
    // B0=Bed off, E=Target temperature
    marlin_client::gcode_printf("M1700 T-1 B0 E W2");
}

/*****************************************************************************/
// MI_PREHEAT_BED
MI_PREHEAT_BED::MI_PREHEAT_BED()
    : MI_event_dispatcher(_(label)) {}

void MI_PREHEAT_BED::Do() {
    // H=Bed only
    marlin_client::gcode_printf("M1700 T-1 H");
}

/*****************************************************************************/
// NsPreheat::MI_COOLDOWN
MI_PREHEAT_COOLDOWN::MI_PREHEAT_COOLDOWN()
    : MI_event_dispatcher(_(label)) {}

void MI_PREHEAT_COOLDOWN::Do() {
    const Response response = filament::get_description(filament::Type::NONE).response;
    marlin_client::FSM_response(PhasesPreheat::UserTempSelection, response);
    marlin_client::gcode("M140 S0");
}
