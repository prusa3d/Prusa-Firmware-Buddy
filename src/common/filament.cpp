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

filament::Description custom_filaments[config_store_ns::max_custom_filament_slots] = {
    { BtnResponse::GetText(Response::CUSTOM_1), 215, 170, 60, Response::CUSTOM_1 },
    { BtnResponse::GetText(Response::CUSTOM_2), 215, 170, 60, Response::CUSTOM_2 },
    { BtnResponse::GetText(Response::CUSTOM_3), 215, 170, 60, Response::CUSTOM_3 },
    { BtnResponse::GetText(Response::CUSTOM_4), 215, 170, 60, Response::CUSTOM_4 }
};

static_assert(sizeof(filaments) / sizeof(filaments[0]) == size_t(filament::Type::_last) + 1, "Filament count error.");

filament::Type filament::get_type(const char *name, size_t name_len) {
    // first name is not valid ("---")
    for (size_t i = size_t(filament::Type::NONE) + 1; i <= size_t(filament::Type::_last); ++i) {
        if ((strlen(filaments[i].name) == name_len) && (!strncmp(name, filaments[i].name, name_len))) {
            return static_cast<filament::Type>(i);
        }
    }
    for (size_t i = 0; i <= size_t(custom_filaments); ++i) {
        if ((strlen(custom_filaments[i].name) == name_len) && (!strncmp(name, custom_filaments[i].name, name_len))) {
            return static_cast<filament::Type>(size_t(filament::Type::_last) + 1 + i);
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
    for (size_t i = 0; i <= size_t(custom_filaments); ++i) {
        if (custom_filaments[i].response == resp) {
            return static_cast<filament::Type>(size_t(filament::Type::_last) + 1 + i);
        }
    }
    return filament::Type::NONE;
}

const filament::Description &filament::get_description(const filament::Type filament) {
    if (filament == filament::Type::CUSTOM_1) {
        custom_filaments[0].name = config_store().custom_filament_name_1.get_c_str();
        custom_filaments[0].nozzle = config_store().custom_filament_nozzle_1.get();
        custom_filaments[0].nozzle_preheat = config_store().custom_filament_nozzle_preheat_1.get();
        custom_filaments[0].heatbed = config_store().custom_filament_heatbed_1.get();
        const filament::Description &filamentsetting = custom_filaments[0];
        return filamentsetting;
    } else if (filament == filament::Type::CUSTOM_2) {
        custom_filaments[1].name = config_store().custom_filament_name_2.get_c_str();
        custom_filaments[1].nozzle = config_store().custom_filament_nozzle_2.get();
        custom_filaments[1].nozzle_preheat = config_store().custom_filament_nozzle_preheat_2.get();
        custom_filaments[1].heatbed = config_store().custom_filament_heatbed_2.get();
        const filament::Description &filamentsetting = custom_filaments[1];
        return filamentsetting;
    } else if (filament == filament::Type::CUSTOM_3) {
        custom_filaments[2].name = config_store().custom_filament_name_3.get_c_str();
        custom_filaments[2].nozzle = config_store().custom_filament_nozzle_3.get();
        custom_filaments[2].nozzle_preheat = config_store().custom_filament_nozzle_preheat_3.get();
        custom_filaments[2].heatbed = config_store().custom_filament_heatbed_3.get();
        const filament::Description &filamentsetting = custom_filaments[2];
        return filamentsetting;
    } else if (filament == filament::Type::CUSTOM_4) {
        custom_filaments[3].name = config_store().custom_filament_name_4.get_c_str();
        custom_filaments[3].nozzle = config_store().custom_filament_nozzle_4.get();
        custom_filaments[3].nozzle_preheat = config_store().custom_filament_nozzle_preheat_4.get();
        custom_filaments[3].heatbed = config_store().custom_filament_heatbed_4.get();
        const filament::Description &filamentsetting = custom_filaments[3];
        return filamentsetting;
    }
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
