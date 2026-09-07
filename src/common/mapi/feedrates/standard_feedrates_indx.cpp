/// @file standard_feedrates_indx.cpp

#include "standard_feedrates.hpp"
#include <utils/enum_array.hpp>

using namespace buddy::standard_feedrates;

namespace {
/// @brief  (mm/s) Extruder feedrates for standard situations
static constexpr EnumArray<Extruder, feedRate_t, Extruder::_count> base_e_feedrate {
    { Extruder::pause_prime, 10.0f },
    { Extruder::retract, 40.0f },
    { Extruder::deretract, 15.0f },
    { Extruder::filament_unload, 15.0f },
    { Extruder::filament_slow_load, 10.0f },
    { Extruder::filament_fast_load, 15.0f },
    { Extruder::filament_assisted, 3.0f },
    { Extruder::advanced_pause_purge, 3.0f },
};
} // namespace

feedRate_t buddy::adjust_feedrate_for_filament(feedRate_t base, const FilamentTypeParameters &filament) {
    constexpr feedRate_t flex_feedrate_factor = 1.f / 6.f;
    return filament.is_flexible ? base * flex_feedrate_factor : base;
}

feedRate_t buddy::standard_feedrates::extruder(Extruder use_case, const FilamentTypeParameters &filament) {
    return buddy::adjust_feedrate_for_filament(base_e_feedrate[use_case], filament);
};
