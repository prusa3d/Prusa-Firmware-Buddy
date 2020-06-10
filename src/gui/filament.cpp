/*
 * filament.cpp
 */

#include "eeprom.h"
#include "assert.h"
#include "filament.h"
#include <cstring>
#include "../lang/i18n.h"

//fixme generating long names, takes too long
const filament_t filaments[FILAMENTS_END] = {
    { "---", N_("Cooldown"), 0, 0 },
    { "PLA", "PLA      215/ 60", 215, 60 },
    { "PETG", "PETG     230/ 85", 230, 85 },
    { "ASA", "ASA      260/100", 260, 100 },
    { "ABS", "ABS      255/100", 255, 100 },
    { "PC", "PC       275/100", 275, 100 },
    { "FLEX", "FLEX     240/ 50", 240, 50 },
    { "HIPS", "HIPS     220/100", 220, 100 },
    { "PP", "PP       240/100", 240, 100 },
};

static_assert(sizeof(filaments) / sizeof(filaments[0]) == FILAMENTS_END, "Filament count error.");

static FILAMENT_t filament_selected = FILAMENTS_END;

extern "C" {

//todo remove this variable after pause refactoring
FILAMENT_t filament_to_load = DEFAULT_FILAMENT;

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

FILAMENT_t get_filament_from_string(const char *s, size_t len) {
    for (size_t i = FILAMENT_NONE + 1; i < FILAMENTS_END; ++i) {
        if ((strlen(filaments[i].name) == len) && (!strncmp(s, filaments[i].name, len))) {
            return static_cast<FILAMENT_t>(i);
        }
    }
    return FILAMENT_NONE;
}

} //extern "C"
