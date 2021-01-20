/*
 * filament.h
 */

#pragma once
#include <stdio.h>

#ifndef __cplusplus
const char *get_selected_filament_name();
#else //__cplusplus
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
    count
};

static const filament_t DEFAULT_FILAMENT = filament_t::PLA;

static const float PREHEAT_TEMP = 170.f;

//todo remove this variable after pause refactoring
extern filament_t filament_to_load;

extern const Filament filaments[size_t(filament_t::count)];

void set_filament(filament_t filament);

filament_t get_filament();
filament_t get_last_preheated_filament();
void set_last_preheated_filament(filament_t filament);

filament_t get_filament_from_string(const char *s, size_t len);
#endif //__cplusplus
