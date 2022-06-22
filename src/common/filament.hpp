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
    uint16_t nozzle_preheat;
    uint16_t heatbed;
    Response response;
};

enum class filament_t {
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
    _last = FLEX
};

class Filaments {
    static filament_t filament_to_load;
    static filament_t filament_last_preheat;

    static filament_t &get_ref();

public:
    using Array = const Filament[size_t(filament_t::_last) + 1];

    static constexpr filament_t Default = filament_t::PLA;
    static constexpr float ColdNozzle = 50.f;
    static constexpr float ColdBed = 45.f;

    static filament_t Find(Response resp);
    static filament_t FindByName(const char *s, size_t len);

    static const Filament &Get(filament_t filament);
    static const Filament &Current();
    static const filament_t CurrentIndex();
    static void Set(filament_t filament);

    static filament_t GetToBeLoaded();
    static void SetToBeLoaded(filament_t filament);

    static filament_t GetLastPreheated();
    static void SetLastPreheated(filament_t filament);
};
