/**
 * @file pause_settings.cpp
 */

#include "pause_settings.hpp"
#include "config_store/store_c_api.h"
#include "../../../lib/Marlin/Marlin/src/core/types.h"
#include "../../../lib/Marlin/Marlin/src/feature/pause.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include <option/has_mmu2.h>

// cannot be class member (externed in marlin)
fil_change_settings_t fc_settings[EXTRUDERS];

using namespace pause;

Settings::Settings()
    : unload_length(GetDefaultUnloadLength())
    , slow_load_length(GetDefaultSlowLoadLength())
    , fast_load_length(GetDefaultFastLoadLength())
    , purge_length(GetDefaultPurgeLength())
    , retract(GetDefaultRetractLength())
    , park_z_feedrate(MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z))
    , park_pos { NAN, NAN, NAN }
    , resume_pos { NAN, NAN, NAN, NAN }
    , target_extruder(0)
    , can_stop(true)
    , do_stop(false)
#if HAS_MMU2()
    , extruder_mmu_rework(config_store().is_mmu_rework.get())
#endif
//
{
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
#if PRINTER_IS_PRUSA_MK4
    // Double the purge length for HF nozzles - presumably on MK4S (for now)
    // This code will change in the future for different nozzle types
    return ((config_store().extended_printer_type.get() == ExtendedPrinterType::mk4s) ? 2 : 1) * ADVANCED_PAUSE_PURGE_LENGTH;
#else
    return ADVANCED_PAUSE_PURGE_LENGTH;
#endif
}

float Settings::GetDefaultRetractLength() {
    return -std::abs(PAUSE_PARK_RETRACT_LENGTH);
}

float Settings::GetDefaultParkZFeedrate() {
    return MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z);
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
    purge_length = std::max(std::abs(len.has_value() ? len.value() : GetDefaultPurgeLength()), (float)minimal_purge);
}

void Settings::SetRetractLength(const std::optional<float> &len) {
    retract = -std::abs(len.has_value() ? len.value() : GetDefaultRetractLength()); // retract is negative
}

void Settings::SetParkZFeedrate(const std::optional<float> &feedrate) {
    if (feedrate.has_value() && std::abs(feedrate.value()) < MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z)) {
        park_z_feedrate = std::abs(feedrate.value());
    } else {
        park_z_feedrate = GetDefaultParkZFeedrate();
    }
}

void Settings::SetParkPoint(const xyz_pos_t &park_point) {
    park_pos = park_point; // TODO check limits
    park_pos.z = std::min(park_pos.z, get_z_max_pos_mm());
}

void Settings::SetResumePoint(const xyze_pos_t &resume_point) {
    resume_pos = resume_point; // TODO check limits
}

void Settings::SetMmuFilamentToLoad(uint8_t index) {
    mmu_filament_to_load = index;
}
