#pragma once
#include <stdio.h>
#include "general_response.hpp"
#include "printers.h"
#include <cstring>

namespace filament {

struct Description {
    uint16_t nozzle;
    uint16_t nozzle_preheat;
    uint16_t heatbed;
    Response response;
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
    _last = PA
};

struct ColorIndex {
    const char *name;
    uint32_t color;
};

enum class ColorName : uint32_t {
    NONE = 0,
    BLACK,
    BLUE,
    GREEN,
    BROWN,
    PURPLE,
    GRAY,
    TERRACOTTA,
    SILVER,
    GOLD,
    RED,
    PINK,
    ORANGE,
    TRANSPARENT,
    YELLOW,
    WHITE,
    _last = WHITE
};

const ColorIndex colortable[size_t(filament::ColorName::_last) + 1] = {
    { "NONE", 0 },
    { "BLACK", 0x000000 },
    { "BLUE", 0x0000FF },
    { "GREEN", 0x00FF00 },
    { "BROWN", 0x800000 },
    { "PURPLE", 0x800080 },
    { "GRAY", 0x999999 },
    { "TERRACOTTA", 0xB87F6A },
    { "SILVER", 0xC0C0C0 },
    { "GOLD", 0xD4AF37 },
    { "RED", 0xFF0000 },
    { "PINK", 0xFF007F },
    { "ORANGE", 0xFF8000 },
    { "TRANSPARENT", 0xF0F0F0 },
    { "YELLOW", 0xFFFF00 },
    { "WHITE", 0xFFFFFF }
};

constexpr Type default_type = Type::PLA;
constexpr float cold_nozzle = 50.f;
constexpr float cold_bed = 45.f;

Type get_type(Response resp);
Type get_type(const char *name, size_t name_len);

const Description &get_description(Type type);
const char *get_name(Type type);

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

    static Colour from_string(char *name) {
        // first name is not valid ("---")
        size_t name_len = strlen(name);
        for (size_t i = size_t(ColorName::NONE) + 1; i <= size_t(ColorName::_last); ++i) {
            if ((strlen(colortable[i].name) == name_len) && (!strncmp(name, colortable[i].name, name_len))) {
                return from_int(colortable[i].color);
            }
        }
        return from_int(atoi(name));
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
