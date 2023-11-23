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

// TODO: unify with the one in gcode_info
struct Colour {
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    int to_int() const {
        return red << 16 | green << 8 | blue;
    }

    static Colour from_int(int value) {
        return Colour {
            .red = static_cast<uint8_t>((value >> 16) & 0xFF),
            .green = static_cast<uint8_t>((value >> 8) & 0xFF),
            .blue = static_cast<uint8_t>(value & 0xFF),
        };
    }
};

std::optional<Colour> get_color_to_load();
void set_color_to_load(std::optional<Colour> color);
}; // namespace filament
