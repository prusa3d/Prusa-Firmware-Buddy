/*
 * filament.cpp
 */

#include "eeprom.h"
#include "assert.h"
#include "filament.hpp"
#include <cstring>
#include "i18n.h"
#include "client_response_texts.hpp"

// only function used in filament.h
const char *get_selected_filament_name() {
    return Filaments::Current().name;
}

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

const Filaments::Array filaments = {
    { "---", BtnResponse::GetText(Response::Cooldown),   0,    0,   0, Response::Cooldown }, // Cooldown sets long text instead short, not a bug
    { BtnResponse::GetText(Response::PLA), pla_str,     215, 170,  60, Response::PLA },
    { BtnResponse::GetText(Response::PETG), pet_g_str,  230, 170,  85, Response::PETG },
    { BtnResponse::GetText(Response::ASA), asa_str,     260, 170, 100, Response::ASA },
    { BtnResponse::GetText(Response::PC), pc_str,       275, 170, 100, Response::PC },
    { BtnResponse::GetText(Response::PVB), pvb_str,     215, 170,  75, Response::PVB },
    { BtnResponse::GetText(Response::ABS), abs_str,     255, 170, 100, Response::ABS },
    { BtnResponse::GetText(Response::HIPS), hips_str,   220, 170, 100, Response::HIPS },
    { BtnResponse::GetText(Response::PP), pp_str,       240, 170, 100, Response::PP },
    { BtnResponse::GetText(Response::FLEX), flex_str,   240, 170,  50, Response::FLEX },
};
// clang-format on

static_assert(sizeof(filaments) / sizeof(filaments[0]) == size_t(filament_t::_last) + 1, "Filament count error.");

filament_t Filaments::filament_last_preheat = filament_t::NONE;
filament_t Filaments::filament_to_load = Filaments::Default; //todo remove this variable after pause refactoring

filament_t Filaments::GetToBeLoaded() {
    return filament_to_load;
}

void Filaments::SetToBeLoaded(filament_t filament) {
    filament_to_load = filament;
}

//first call will initialize variable from flash, similar behavior to Meyers singleton
filament_t &Filaments::get_ref() {
    static filament_t filament_selected = filament_t(eeprom_get_ui8(EEVAR_FILAMENT_TYPE));
    if (size_t(filament_selected) > size_t(filament_t::_last)) {
        filament_selected = filament_t::NONE;
    }
    return filament_selected;
}

// first name is not valid ("---")
filament_t Filaments::FindByName(const char *s, size_t len) {
    for (size_t i = size_t(filament_t::NONE) + 1; i <= size_t(filament_t::_last); ++i) {
        if ((strlen(filaments[i].name) == len) && (!strncmp(s, filaments[i].name, len))) {
            return static_cast<filament_t>(i);
        }
    }
    return filament_t::NONE;
}

filament_t Filaments::Find(Response resp) {
    for (size_t i = size_t(filament_t::NONE); i <= size_t(filament_t::_last); ++i) {
        if (filaments[i].response == resp) {
            return static_cast<filament_t>(i);
        }
    }
    return filament_t::NONE;
}

const Filament &Filaments::Get(filament_t filament) {
    return filaments[size_t(filament)];
}

const Filament &Filaments::Current() {
    return Get(CurrentIndex());
}

const filament_t Filaments::CurrentIndex() {
    return get_ref();
}

void Filaments::Set(filament_t filament) {
    assert(filament <= filament_t::_last);
    if (filament == get_ref()) {
        return;
    }
    get_ref() = filament;
    eeprom_set_ui8(EEVAR_FILAMENT_TYPE, size_t(filament));
}

filament_t Filaments::GetLastPreheated() {
    return filament_last_preheat;
}
void Filaments::SetLastPreheated(filament_t filament) {
    filament_last_preheat = filament;
}
