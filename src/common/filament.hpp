/**
 * @file filament.hpp
 * @author Radek Vana
 * @brief
 * @date 2021-01-20
 */

#pragma once
#include <stdio.h>
#include "general_response.hpp"

struct Filament {
    const char *name;
    const char *long_name;
    uint16_t nozzle;
    uint16_t heatbed;
    Response response;
};

enum class filament_t {
    NONE = 0,
    PLA,
    PETG,
    ASA,
    ABS,
    PC,
    FLEX,
    HIPS,
    PP,
    _last = PP
};

static const filament_t DEFAULT_FILAMENT = filament_t::PLA;

static const float PREHEAT_TEMP = 170.f;

//todo remove this variable after pause refactoring
extern filament_t filament_to_load;

extern const Filament filaments[size_t(filament_t::_last) + 1];

void set_filament(filament_t filament);

filament_t get_filament();
filament_t get_last_preheated_filament();
void set_last_preheated_filament(filament_t filament);

filament_t get_filament_from_string(const char *s, size_t len);

class Filaments {
public:
    static filament_t Find(Response resp);
    static const Filament &Get(filament_t filament);
};
