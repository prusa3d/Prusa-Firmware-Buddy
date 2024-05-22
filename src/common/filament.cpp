#include "assert.h"
#include "filament.hpp"
#include "i18n.h"
#include "client_response_texts.hpp"
#include "../../include/printers.h"
#include <Marlin/src/inc/MarlinConfigPre.h>

#include <cstring>
#include <option/has_loadcell.h>

// These temperatures correspond to slicer defaults for MBL.
constexpr uint16_t PC_NOZZLE_PREHEAT = HAS_LOADCELL() ? 170 : 275 - 25;
constexpr uint16_t FLEX_NOZZLE_PREHEAT = HAS_LOADCELL() ? 170 : 210;

// MINI has slightly lower max nozzle temperature but it is still OK for polyamid
constexpr uint16_t PA_NOZZLE = PRINTER_IS_PRUSA_MINI ? 280 : 285;

constexpr filament::Description filaments[size_t(filament::Type::_last) + 1] = {
    { 0, 0, 0, Response::Cooldown },
    { 215, 170, 60, Response::PLA },
    { 230, 170, 85, Response::PETG },
    { 260, 170, 100, Response::ASA },
    { 275, PC_NOZZLE_PREHEAT, 100, Response::PC },
    { 215, 170, 75, Response::PVB },
    { 255, 170, 100, Response::ABS },
    { 220, 170, 100, Response::HIPS },
    { 240, 170, 100, Response::PP },
    { PA_NOZZLE, 170, 100, Response::PA },
    { 240, FLEX_NOZZLE_PREHEAT, 50, Response::FLEX },
};

static_assert(sizeof(filaments) / sizeof(filaments[0]) == size_t(filament::Type::_last) + 1, "Filament count error.");

constexpr bool temperatures_are_within_spec(filament::Description filament) {
    return (filament.nozzle <= HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.nozzle_preheat <= HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.heatbed <= BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN);
}

static_assert(std::ranges::all_of(filaments, temperatures_are_within_spec));

filament::Type filament::get_type(const char *name, size_t name_len) {
    // first name is not valid ("---")
    for (size_t i = size_t(filament::Type::NONE) + 1; i <= size_t(filament::Type::_last); ++i) {
        const char *filament_name = get_response_text(filaments[i].response);
        if ((strlen(filament_name) == name_len) && (!strncmp(name, filament_name, name_len))) {
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

const char *filament::get_name(Type type) {
    if (type == Type::NONE) {
        return "---";
    }
    const Description &description = get_description(type);
    return get_response_text(description.response);
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
