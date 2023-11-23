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
#if PRINTER_IS_PRUSA_iX
    { BtnResponse::GetText(Response::PETG_NH), 230, 170, 0, Response::PETG_NH },
#endif // PRINTER_IS_PRUSA_iX
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
    { BtnResponse::GetText(Response::PA), 285, 170, 100, Response::PA },
#if HAS_LOADCELL()
    { BtnResponse::GetText(Response::FLEX), 240, 170, 50, Response::FLEX },
#else
    { BtnResponse::GetText(Response::FLEX), 240, 210, 50, Response::FLEX },
#endif
};

static_assert(sizeof(filaments) / sizeof(filaments[0]) == size_t(filament::Type::_last) + 1, "Filament count error.");

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

filament::Type filament::get_type_to_load() {
    return filament_to_load;
}

void filament::set_type_to_load(filament::Type filament) {
    filament_to_load = filament;
}

static std::optional<filament::Colour> color_to_load { std::nullopt };

std::optional<filament::Colour> filament::get_color_to_load() {
    return color_to_load;
}

void filament::set_color_to_load(std::optional<filament::Colour> color) {
    color_to_load = color;
}
