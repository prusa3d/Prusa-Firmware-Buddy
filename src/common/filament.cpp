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

// only function used in filament.h
const char *get_selected_filament_name() {
    return Filaments::Current().name;
}

#if defined(USE_ST7789)
    #define SPACES "" // no extra spaces
#endif                // USE_<display>

// clang-format off
const Filaments::Array filaments = {
    { "---",                                  0,   0,   0, Response::Cooldown },
    { BtnResponse::GetText(Response::PLA),  215, 170,  60, Response::PLA },
    { BtnResponse::GetText(Response::PETG), 230, 170,  85, Response::PETG },
    { BtnResponse::GetText(Response::ASA),  260, 170, 100, Response::ASA },
    { BtnResponse::GetText(Response::PC),   275, 170, 100, Response::PC },
    { BtnResponse::GetText(Response::PVB),  215, 170,  75, Response::PVB },
    { BtnResponse::GetText(Response::ABS),  255, 170, 100, Response::ABS },
    { BtnResponse::GetText(Response::HIPS), 220, 170, 100, Response::HIPS },
    { BtnResponse::GetText(Response::PP),   240, 170, 100, Response::PP },
    { BtnResponse::GetText(Response::FLEX), 240, 170,  50, Response::FLEX },
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
