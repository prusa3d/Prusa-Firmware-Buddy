/*
 * filament.cpp
 */

#include "eeprom.h"
#include "assert.h"
#include "filament.h"
#include <cstring>
#include "i18n.h"

// clang-format off
// to keep the texts aligned for easier checking of their alignment on the LCD
static constexpr const char *pla_str =   "PLA      215/ 60";
static constexpr const char *pet_g_str = "PETG     230/ 85";
static constexpr const char *asa_str =   "ASA      260/100";
static constexpr const char *pc_str =    "PC       275/100";
static constexpr const char *pvb_str =   "PVB      215/ 75";
static constexpr const char *abs_str =   "ABS      255/100";
static constexpr const char *hips_str =  "HIPS     220/100";
static constexpr const char *pp_str =    "PP       240/100";
static constexpr const char *flex_str =  "FLEX     240/ 50";
// clang-format on
//fixme generating long names, takes too long
const filament_t filaments[FILAMENTS_END] = {
    { "---", N_("Cooldown"), 0, 0 },
    { "PLA", pla_str, 215, 60 },
    { "PETG", pet_g_str, 230, 85 },
    { "ASA", asa_str, 260, 100 },
    { "PC", pc_str, 275, 100 },
    { "PVB", pvb_str, 215, 75 },
    { "ABS", abs_str, 255, 100 },
    { "HIPS", hips_str, 220, 100 },
    { "PP", pp_str, 240, 100 },
    { "FLEX", flex_str, 240, 50 },
};

static_assert(sizeof(filaments) / sizeof(filaments[0]) == FILAMENTS_END, "Filament count error.");

static FILAMENT_t filament_selected = FILAMENTS_END;
static FILAMENT_t filament_last_preheat = FILAMENT_NONE;

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
        uint8_t fil = variant_get_ui8(eeprom_get_var(EEVAR_FILAMENT_TYPE));
        if (fil >= FILAMENTS_END)
            fil = 0;
        filament_selected = (FILAMENT_t)fil;
    }
    return filament_selected;
}

FILAMENT_t get_last_preheated_filament() {
    return filament_last_preheat;
}

void set_last_preheated_filament(FILAMENT_t filament) {
    filament_last_preheat = filament;
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
