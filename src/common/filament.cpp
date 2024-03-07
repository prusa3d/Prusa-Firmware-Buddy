#include "assert.h"
#include "filament.hpp"
#include "custom_filament_tools.hpp"
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

filament::Description custom_filaments[config_store_ns::max_custom_filament_slots] = {
    { 215, 170, 60, Response::CUSTOM_1 },
    { 215, 170, 60, Response::CUSTOM_2 },
    { 215, 170, 60, Response::CUSTOM_3 },
    { 215, 170, 60, Response::CUSTOM_4 }
};

static_assert(sizeof(filaments) / sizeof(filaments[0]) == size_t(filament::Type::_last) + 1, "Filament count error.");

constexpr bool temperatures_are_within_spec(filament::Description filament) {
    return (filament.nozzle <= HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.nozzle_preheat <= HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.heatbed <= BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN);
}

static_assert(std::ranges::all_of(filaments, temperatures_are_within_spec));

filament::Type filament::get_type(Response resp) {
    for (size_t i = size_t(filament::Type::NONE); i <= size_t(filament::Type::_last); ++i) {
        if (filaments[i].response == resp) {
            return static_cast<filament::Type>(i);
        }
    }
    for (size_t i = 0; i <= size_t(custom_filaments); ++i) {
        if (custom_filaments[i].response == resp) {
            return static_cast<filament::Type>(size_t(filament::Type::_last) + 1 + i);
        }
    }
    return filament::Type::NONE;
}

const filament::Description &filament::get_description(const filament::Type filament) {
    uint8_t slot = custom_filament_tools::SlotToIndex(custom_filament_tools::CustomFilamentSlots::END);
    if (filament == filament::Type::CUSTOM_1) {
        slot = custom_filament_tools::SlotToIndex(custom_filament_tools::CustomFilamentSlots::CUSTOM_1);
    } else if (filament == filament::Type::CUSTOM_2) {
        slot = custom_filament_tools::SlotToIndex(custom_filament_tools::CustomFilamentSlots::CUSTOM_2);
    } else if (filament == filament::Type::CUSTOM_3) {
        slot = custom_filament_tools::SlotToIndex(custom_filament_tools::CustomFilamentSlots::CUSTOM_3);
    } else if (filament == filament::Type::CUSTOM_4) {
        slot = custom_filament_tools::SlotToIndex(custom_filament_tools::CustomFilamentSlots::CUSTOM_4);
    }
    if (slot == custom_filament_tools::SlotToIndex(custom_filament_tools::CustomFilamentSlots::END)) {
        return filaments[size_t(filament)];
    } else {
        custom_filaments[slot].nozzle = config_store().custom_filament_temps.get()[slot].at(custom_filament_tools::TempToIndex(custom_filament_tools::CustomFilamentTemperatures::nozzle));
        custom_filaments[slot].nozzle_preheat = config_store().custom_filament_temps.get()[slot].at(custom_filament_tools::TempToIndex(custom_filament_tools::CustomFilamentTemperatures::nozzle_preheat));
        custom_filaments[slot].heatbed = config_store().custom_filament_temps.get()[slot].at(custom_filament_tools::TempToIndex(custom_filament_tools::CustomFilamentTemperatures::heatbed));
        const filament::Description &filamentsetting = custom_filaments[slot];
        return filamentsetting;
    }
}

filament::Type filament::get_type(const char *name, size_t name_len) {
    // first name is not valid ("---")
    for (size_t i = size_t(filament::Type::NONE) + 1; i <= size_t(filament::Type::_last); ++i) {
        const char *filament_name = get_response_text(filaments[i].response);
        if ((strlen(filament_name) == name_len) && (!strncmp(name, filament_name, name_len))) {
            return static_cast<filament::Type>(i);
        }
    }
    return static_cast<filament::Type>(filament::Type::NONE);
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
