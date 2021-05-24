/*
 * filament.cpp
 */

#include "eeprom.h"
#include "assert.h"
#include "filament.h"
#include "filament.hpp"
#include <cstring>
#include "i18n.h"

#if HAS_GUI
    #include "dialog_response.hpp"
#else
class BtnTexts {
public:
    static constexpr const char *Get(Response resp) { return ""; };
};
#endif

// only function used in filament.h
extern "C" const char *get_selected_filament_name() {
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
// clang-format on

//not a member - static_assert cannot access private members
//fixme generating long names, takes too long
const Filaments::Array filaments = {
    { "---", BtnTexts::Get(Response::Cooldown), 0, 0, Response::Cooldown }, // Cooldown sets long text instead short, not a bug
    { BtnTexts::Get(Response::PLA), pla_str, 215, 60, Response::PLA },
    { BtnTexts::Get(Response::PETG), pet_g_str, 230, 85, Response::PETG },
    { BtnTexts::Get(Response::ASA), asa_str, 260, 100, Response::ASA },
    { BtnTexts::Get(Response::ABS), abs_str, 255, 100, Response::ABS },
    { BtnTexts::Get(Response::PC), pc_str, 275, 100, Response::PC },
    { BtnTexts::Get(Response::FLEX), flex_str, 240, 50, Response::FLEX },
    { BtnTexts::Get(Response::HIPS), hips_str, 220, 100, Response::HIPS },
    { BtnTexts::Get(Response::PP), pp_str, 240, 100, Response::PP },
};

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
    static filament_t filament_selected = filament_t(variant_get_ui8(eeprom_get_var(EEVAR_FILAMENT_TYPE)));
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
    eeprom_set_var(EEVAR_FILAMENT_TYPE, variant8_ui8(size_t(filament)));
}

filament_t Filaments::GetLastPreheated() {
    return filament_last_preheat;
}
void Filaments::SetLastPreheated(filament_t filament) {
    filament_last_preheat = filament;
}
