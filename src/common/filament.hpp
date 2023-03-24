#pragma once
#include <stdio.h>
#include "general_response.hpp"
#include "printers.h"

namespace filament {

struct Description {
    const char *name;
    uint16_t nozzle;
    uint16_t nozzle_preheat;
    uint16_t heatbed;
    Response response;
};

enum class Type {
    NONE = 0,
    PLA,
    PETG,
#if (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    PETG_NH,
#endif
    ASA,
    PC,
    PVB,
    ABS,
    HIPS,
    PP,
    FLEX,
    PA,
    _last = PA
};

constexpr Type default_type = Type::PLA;
constexpr float cold_nozzle = 50.f;
constexpr float cold_bed = 45.f;

const Type get_type_in_extruder(uint8_t extruder);
void set_type_in_extruder(Type filament, uint8_t extruder);

Type get_type(Response resp);
Type get_type(const char *name, size_t name_len);

const Description &get_description(Type type);

Type get_type_to_load();
void set_type_to_load(Type filament);

Type get_type_last_preheated();
void set_type_last_preheated(Type filament);
};
