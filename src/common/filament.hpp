#pragma once
#include <stdio.h>
#include "marlin_server_types/general_response.hpp"
#include "printers.h"
#include <cstring>
#include <color.hpp>

namespace filament {

struct Description {
    uint16_t nozzle_temperature;
    uint16_t nozzle_preheat_temperature;
    uint16_t heatbed_temperature;
    const char *name;
};

enum class Type : uint8_t {
    NONE = 0,
    PLA,
    PETG,
    ASA,
    PC,
    PVB,
    ABS,
    HIPS,
    PP,
    FLEX,
    PA,

    _last = PA,
    none = NONE,
};

constexpr Type default_type = Type::PLA;
constexpr float cold_nozzle = 50.f;
constexpr float cold_bed = 45.f;

Type get_type(const char *name, size_t name_len);

const Description &get_description(Type type);
const char *get_name(Type type);

}; // namespace filament

// This is here to reduce changes for the future PRs. It will be removed soon.
using FilamentType = filament::Type;
using PresetFilamentType = FilamentType;
using FilamentTypeParameters = filament::Description;
