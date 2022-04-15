/**
 * @file pause_settings.cpp
 */

#include "pause_settings.hpp"
#include "config_features.h"
#include "eeprom_function_api.h"
#include "../../../lib/Marlin/Marlin/src/core/types.h"
#include "../../../lib/Marlin/Marlin/src/feature/pause.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"

//cannot be class member (externed in marlin)
fil_change_settings_t fc_settings[EXTRUDERS];

using namespace pause;

Settings::Settings()
    : unload_length(GetDefaultUnloadLength())
    , slow_load_length(GetDefaultSlowLoadLength())
    , fast_load_length(GetDefaultFastLoadLength())
    , purge_length(GetDefaultPurgeLength())
    , retract(GetDefaultRetractLength())
    , park_pos { NAN, NAN, NAN }
    , resume_pos { NAN, NAN, NAN, NAN }
    , target_extruder(0)
    , can_stop(true)
    , do_stop(false) {
}

float Settings::GetDefaultFastLoadLength() {
    return fc_settings[active_extruder].load_length;
}

float Settings::GetDefaultSlowLoadLength() {
    return FILAMENT_CHANGE_SLOW_LOAD_LENGTH;
}

float Settings::GetDefaultUnloadLength() {
    return fc_settings[active_extruder].unload_length;
}

float Settings::GetDefaultPurgeLength() {
    return ADVANCED_PAUSE_PURGE_LENGTH;
}

float Settings::GetDefaultRetractLength() {
    return PAUSE_PARK_RETRACT_LENGTH;
}

void Settings::SetUnloadLength(float len) {
    unload_length = -std::abs(isnan(len) ? GetDefaultUnloadLength() : len); // it is negative value
}

void Settings::SetSlowLoadLength(float len) {
    slow_load_length = std::abs(isnan(len) ? GetDefaultSlowLoadLength() : len);
}

void Settings::SetFastLoadLength(float len) {
    fast_load_length = std::abs(isnan(len) ? GetDefaultFastLoadLength() : len);
}

void Settings::SetPurgeLength(float len) {
    purge_length = std::max(std::abs(isnan(len) ? GetDefaultPurgeLength() : len), (float)minimal_purge);
}

void Settings::SetRetractLength(float len) {
    retract = -std::abs(isnan(len) ? GetDefaultRetractLength() : len); // retract is negative
}

void Settings::SetParkPoint(const xyz_pos_t &park_point) {
    park_pos = park_point; //TODO check limits
    park_pos.z = std::min(park_pos.z, get_z_max_pos_mm());
}

void Settings::SetResumePoint(const xyze_pos_t &resume_point) {
    resume_pos = resume_point; //TODO check limits
}

void Settings::SetMmuFilamentToLoad(uint8_t index) {
    mmu_filament_to_load = index;
}
