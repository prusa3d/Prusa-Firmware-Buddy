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

enum class Type : uint8_t {
    NONE = 0,
    PLA,
    PETG,
#if PRINTER_IS_PRUSA_iX
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

Type get_type(Response resp);
Type get_type(const char *name, size_t name_len);

const Description &get_description(Type type);

Type get_type_to_load();
void set_type_to_load(Type filament);
};
