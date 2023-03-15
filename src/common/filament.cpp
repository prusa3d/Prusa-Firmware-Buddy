#include "eeprom.h"
#include "assert.h"
#include "filament.hpp"
#include "i18n.h"
#include "client_response_texts.hpp"
#include "../guiconfig/guiconfig.h"
#include "../../include/printers.h"

#include <cstring>
#include <option/has_loadcell.h>

const filament::Description filaments[size_t(filament::Type::_last) + 1] = {
    { "---", 0, 0, 0, Response::Cooldown },
    { BtnResponse::GetText(Response::PLA), 215, 170, 60, Response::PLA },
    { BtnResponse::GetText(Response::PETG), 230, 170, 85, Response::PETG },
#if (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    { BtnResponse::GetText(Response::PETG_NH), 230, 170, 0, Response::PETG_NH },
#endif // PRINTER_PRUSA_IXL
    { BtnResponse::GetText(Response::ASA), 260, 170, 100, Response::ASA },
#if HAS_LOADCELL()
    { BtnResponse::GetText(Response::PC), 275, 170, 100, Response::PC },
#else
    { BtnResponse::GetText(Response::PC), 275, 275 - 25, 100, Response::PC },
#endif
    { BtnResponse::GetText(Response::PVB), 215, 170, 75, Response::PVB },
    { BtnResponse::GetText(Response::ABS), 255, 170, 100, Response::ABS },
    { BtnResponse::GetText(Response::HIPS), 220, 170, 100, Response::HIPS },
    { BtnResponse::GetText(Response::PP), 240, 170, 100, Response::PP },
#if HAS_LOADCELL()
    { BtnResponse::GetText(Response::FLEX), 240, 170, 50, Response::FLEX },
#else
    { BtnResponse::GetText(Response::FLEX), 240, 210, 50, Response::FLEX },
#endif
};

static_assert(sizeof(filaments) / sizeof(filaments[0]) == size_t(filament::Type::_last) + 1, "Filament count error.");

static eevar_id get_eevar_id_for_extruder(uint8_t extruder) {
    if (extruder == 0) {
        return EEVAR_FILAMENT_TYPE;
    } else {
        return static_cast<eevar_id>(static_cast<int>(EEVAR_FILAMENT_TYPE_1) + extruder - 1);
    }
}

const filament::Type filament::get_type_in_extruder(uint8_t extruder) {
    auto type = static_cast<filament::Type>(eeprom_get_ui8(get_eevar_id_for_extruder(extruder)));
    if (type > filament::Type::_last) {
        type = filament::Type::NONE;
    }
    return type;
}

void filament::set_type_in_extruder(filament::Type type, uint8_t extruder) {
    assert(extruder <= 5); // we do support the 6th extruder here, but don't want to really use it for now
    eeprom_set_ui8(get_eevar_id_for_extruder(extruder), static_cast<uint8_t>(type));
}

filament::Type filament::get_type(const char *name, size_t name_len) {
    // first name is not valid ("---")
    for (size_t i = size_t(filament::Type::NONE) + 1; i <= size_t(filament::Type::_last); ++i) {
        if ((strlen(filaments[i].name) == name_len) && (!strncmp(name, filaments[i].name, name_len))) {
            return static_cast<filament::Type>(i);
        }
    }
    return filament::Type::NONE;
}

filament::Type filament::get_type(Response resp) {
    for (size_t i = size_t(filament::Type::NONE); i <= size_t(filament::Type::_last); ++i) {
        if (filaments[i].response == resp) {
            return static_cast<filament::Type>(i);
        }
    }
    return filament::Type::NONE;
}

const filament::Description &filament::get_description(filament::Type filament) {
    return filaments[size_t(filament)];
}

static filament::Type filament_to_load = filament::Type::NONE;
static filament::Type filament_last_preheat = filament::default_type;

filament::Type filament::get_type_to_load() {
    return filament_to_load;
}

void filament::set_type_to_load(filament::Type filament) {
    filament_to_load = filament;
}

filament::Type filament::get_type_last_preheated() {
    return filament_last_preheat;
}

void filament::set_type_last_preheated(filament::Type filament) {
    filament_last_preheat = filament;
}
