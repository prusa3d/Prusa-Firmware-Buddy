/*
 * filament.cpp
 */

#include "eeprom.h"
#include "assert.h"
#include "filament.h"
#include <cstring>
#include "i18n.h"
#include "dialog_response.hpp"

extern "C" {
const char *get_selected_filament_name() {
    return filaments[size_t(get_filament())].name;
}
} //extern "C"

static constexpr const char *pla_str = "PLA      215/ 60";
static constexpr const char *pet_g_str = "PETG     230/ 85";
static constexpr const char *asa_str = "ASA      260/100";
static constexpr const char *abs_str = "ABS      255/100";
static constexpr const char *pc_str = "PC       275/100";
static constexpr const char *flex_str = "FLEX     240/ 50";
static constexpr const char *hips_str = "HIPS     220/100";
static constexpr const char *pp_str = "PP       240/100";

//fixme generating long names, takes too long
const Filament filaments[size_t(filament_t::count)] = {
    { "---", BtnTexts::Get(Response::Cooldown), 0, 0, Response::Cooldown },
    { BtnTexts::Get(Response::PLA), pla_str, 215, 60, Response::PLA },
    { BtnTexts::Get(Response::PETG), pet_g_str, 230, 85, Response::PETG },
    { BtnTexts::Get(Response::ASA), asa_str, 260, 100, Response::ASA },
    { BtnTexts::Get(Response::ABS), abs_str, 255, 100, Response::ABS },
    { BtnTexts::Get(Response::PC), pc_str, 275, 100, Response::PC },
    { BtnTexts::Get(Response::FLEX), flex_str, 240, 50, Response::FLEX },
    { BtnTexts::Get(Response::HIPS), hips_str, 220, 100, Response::HIPS },
    { BtnTexts::Get(Response::PP), pp_str, 240, 100, Response::PP },
};

static_assert(sizeof(filaments) / sizeof(filaments[0]) == size_t(filament_t::count), "Filament count error.");

static filament_t filament_selected = filament_t::count;
static filament_t filament_last_preheat = filament_t::NONE;

//todo remove this variable after pause refactoring
filament_t filament_to_load = DEFAULT_FILAMENT;

void set_filament(filament_t filament) {
    assert(filament < filament_t::count);
    if (filament == filament_selected) {
        return;
    }
    filament_selected = filament;
    eeprom_set_var(EEVAR_FILAMENT_TYPE, variant8_ui8(size_t(filament)));
}

filament_t get_filament() {
    if (filament_selected == filament_t::count) {
        size_t fil = variant_get_ui8(eeprom_get_var(EEVAR_FILAMENT_TYPE));
        if (fil >= size_t(filament_t::count))
            fil = 0;
        filament_selected = (filament_t)fil;
    }
    return filament_selected;
}

filament_t get_last_preheated_filament() {
    return filament_last_preheat;
}

void set_last_preheated_filament(filament_t filament) {
    filament_last_preheat = filament;
}

filament_t get_filament_from_string(const char *s, size_t len) {
    for (size_t i = size_t(filament_t::NONE) + 1; i < size_t(filament_t::count); ++i) {
        if ((strlen(filaments[size_t(i)].name) == len) && (!strncmp(s, filaments[size_t(i)].name, len))) {
            return static_cast<filament_t>(i);
        }
    }
    return filament_t::NONE;
}
