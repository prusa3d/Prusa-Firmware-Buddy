/*
 * filament.c
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */
#include "eeprom.h"
#include "assert.h"
#include "dbg.h"
#include "marlin_client.h"
#include "filament.h"
#include "gui.h"

#include "../Marlin/src/gcode/gcode.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/lcd/extensible_ui/ui_api.h"

//fixme generating long names, takes too long
const filament_t filaments[FILAMENTS_END] = {
    { "---", "---", 0, 0 },
    { "PLA", "PLA      215/ 60", 215, 60 },
    { "PETG", "PETG     230/ 85", 230, 85 },
    { "ASA", "ASA      260/100", 260, 100 },
    { "FLEX", "FLEX     240/ 50", 240, 50 },
};

#define FILAMENT_ADDRESS 0x400

static FILAMENT_t filament_selected = FILAMENTS_END;

void set_filament(FILAMENT_t filament) {
    assert(filament < FILAMENTS_END);
    if (filament == filament_selected) {
        return;
    }
    filament_selected = filament;
    eeprom_set_var(EEVAR_FILAMENT_TYPE, variant8_ui8(filament));
}

FILAMENT_t get_filament() {
    if (filament_selected == FILAMENTS_END) {
        uint8_t fil = eeprom_get_var(EEVAR_FILAMENT_TYPE).ui8;
        if (fil >= FILAMENTS_END)
            fil = 0;
        filament_selected = (FILAMENT_t)fil;
    }
    return filament_selected;
}
