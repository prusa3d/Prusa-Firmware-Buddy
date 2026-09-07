/**
 * @file pause_settings.cpp
 */

#include "pause_settings.hpp"
#include "config_features.h"
#include "../../../lib/Marlin/Marlin/src/core/types.h"
#include "../../../lib/Marlin/Marlin/src/feature/pause.h"
#include <option/has_mmu2.h>

// cannot be class member (externed in marlin)
StrongIndexArray<fil_change_settings_t, EXTRUDERS, VirtualToolIndex, VirtualToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> fc_settings;

using namespace pause;

Settings::Settings()
    : unload_length(GetDefaultUnloadLength())
    , slow_load_length(GetDefaultSlowLoadLength())
    , fast_load_length(GetDefaultFastLoadLength())
    , retract(GetDefaultRetractLength())
    , resume_pos { NAN, NAN, NAN, NAN }
    , target_extruder(0)
//
{
}

float Settings::GetDefaultFastLoadLength() {
    const VirtualToolIndex tool = VirtualToolIndex::currently_selected_opt()
                                      .value_or(VirtualToolIndex::from_raw(0));
    return fc_settings[tool].load_length;
}

float Settings::GetDefaultSlowLoadLength() {
    return FILAMENT_CHANGE_SLOW_LOAD_LENGTH;
}

float Settings::GetDefaultUnloadLength() {
    const VirtualToolIndex tool = VirtualToolIndex::currently_selected_opt()
                                      .value_or(VirtualToolIndex::from_raw(0));
    return fc_settings[tool].unload_length;
}

float Settings::GetDefaultPurgeLength(uint8_t extruder) {
    // Double the purge length for HF nozzles
    return ADVANCED_PAUSE_PURGE_LENGTH * (config_store().get_nozzle_is_high_flow(ENABLED(SINGLENOZZLE) ? 0 : extruder) ? 2 : 1);
}

float Settings::GetDefaultRetractLength() {
    return -std::abs(STANDARD_RETRACT_LENGTH);
}

void Settings::SetUnloadLength(const std::optional<float> &len) {
    unload_length = -std::abs(len.has_value() ? len.value() : GetDefaultUnloadLength()); // it is negative value
}

void Settings::SetSlowLoadLength(const std::optional<float> &len) {
    slow_load_length = std::abs(len.has_value() ? len.value() : GetDefaultSlowLoadLength());
}

void Settings::SetFastLoadLength(const std::optional<float> &len) {
    fast_load_length = std::abs(len.has_value() ? len.value() : GetDefaultFastLoadLength());
}

void Settings::SetPurgeLength(const std::optional<float> &len) {
    purge_length_ = len;
}

void Settings::SetRetractLength(const std::optional<float> &len) {
    retract = -std::abs(len.has_value() ? len.value() : GetDefaultRetractLength()); // retract is negative
}

void Settings::SetParkPoint(const mapi::ParkingPosition &park_point) {
    this->park_point = park_point;
}

void Settings::SetResumePoint(const xyze_pos_t &resume_point) {
    resume_pos = resume_point; // TODO check limits
}

void Settings::SetMmuFilamentToLoad(uint8_t index) {
    mmu_filament_to_load = index;
}

void Settings::SetResumeNozzleTemperature(int16_t temperature) {
    resume_nozzle_temperature = temperature;
}

float pause::Settings::purge_length() const {
    return std::max<float>(std::abs(purge_length_.value_or(GetDefaultPurgeLength(ENABLED(SINGLENOZZLE) ? 0 : target_extruder))), minimal_purge);
}
