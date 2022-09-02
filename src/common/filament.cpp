/*
 * filament.cpp
 */

#include "eeprom.h"
#include "assert.h"
#include "filament.hpp"
#include "i18n.h"
#include "client_response_texts.hpp"
#include "../guiconfig/guiconfig.h"
#include "../../include/printers.h"

#include <cstring>
#include "configuration_store.hpp"

// only function used in filament.h
const char *get_selected_filament_name() {
    return Filaments::Current().name;
}

#if defined(USE_ST7789)
    #define SPACES "" // no extra spaces
#endif                // USE_<display>

// clang-format off
// to keep the texts aligned for easier checking of their alignment on the LCD
static constexpr const char *pla_str =      "PLA      " SPACES "215/ 60";
static constexpr const char *pet_g_str =    "PETG     " SPACES "230/ 85";
static constexpr const char *pet_g_nbh_str ="PETG_NH  " SPACES "230/  0";
static constexpr const char *asa_str =      "ASA      " SPACES "260/100";
static constexpr const char *pc_str =       "PC       " SPACES "275/100";
static constexpr const char *pvb_str =      "PVB      " SPACES "215/ 75";
static constexpr const char *abs_str =      "ABS      " SPACES "255/100";
static constexpr const char *hips_str =     "HIPS     " SPACES "220/100";
static constexpr const char *pp_str =       "PP       " SPACES "240/100";
static constexpr const char *flex_str =     "FLEX     " SPACES "240/ 50";

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
filament_t Filaments::filament_to_load = Filaments::Default; // todo remove this variable after pause refactoring

filament_t Filaments::GetToBeLoaded() {
    return filament_to_load;
}

void Filaments::SetToBeLoaded(filament_t filament) {
    filament_to_load = filament;
}

// first call will initialize variable from flash, similar behavior to Meyers singleton
filament_t &Filaments::get_ref() {
    static filament_t filament_selected = static_cast<filament_t>(config_store().filament_type.get());
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
    config_store().filament_type.set(static_cast<uint8_t>(filament));
}

filament_t Filaments::GetLastPreheated() {
    return filament_last_preheat;
}
void Filaments::SetLastPreheated(filament_t filament) {
    filament_last_preheat = filament;
}
