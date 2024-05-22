/// @file globals.cpp
#include "globals.h"
#include "../config/config.h"
#include "permanent_storage.h"
#include "../hal/progmem.h"

namespace modules {
namespace globals {

Globals globals;

void Globals::Init() {
    if (!mps::FilamentLoaded::get(activeSlot)) {
        activeSlot = config::toolCount;
    }

    if (activeSlot < config::toolCount) {
        // some valid slot has been recorded in EEPROM - we have some filament loaded in the selector or even in the nozzle
        filamentLoaded = FilamentLoadState::InNozzle; // let's assume the filament is down to the nozzle as a worst case scenario
    } else {
        // the filament is not present in the selector - we can move the selector freely
        filamentLoaded = FilamentLoadState::AtPulley;
    }

    ResetFSensorToNozzle();
    ResetFSensorUnloadCheck();

    ResetPulleyLoadFeedrate();
    ResetPulleySlowFeedrate();
    ResetPulleyUnloadFeedrate();

    ResetSelectorFeedrate();
    ResetSelectorHomingFeedrate();

    ResetIdlerFeedrate();
    ResetIdlerHomingFeedrate();

    ResetCutIRunCurrent();
    ResetCutLength();
}

uint8_t Globals::ActiveSlot() const {
    return activeSlot;
}

void Globals::SetActiveSlot(uint8_t newActiveSlot) {
    activeSlot = newActiveSlot;
}

FilamentLoadState Globals::FilamentLoaded() const {
    return filamentLoaded;
}

void __attribute__((noinline)) Globals::SetFilamentLoaded(uint8_t slot, FilamentLoadState newFilamentLoaded) {
    filamentLoaded = newFilamentLoaded;
    SetActiveSlot(slot);
    switch (newFilamentLoaded) {
    case FilamentLoadState::NotLoaded:
    case FilamentLoadState::AtPulley:
        // Clear the active slot (basically sets the active slot to config::toolCount)
        mps::FilamentLoaded::set(config::toolCount);
        break;
    case FilamentLoadState::InSelector:
    case FilamentLoadState::InFSensor:
    case FilamentLoadState::InNozzle:
        // Record a valid active slot
        mps::FilamentLoaded::set(slot);
        break;
    }
}

uint16_t Globals::DriveErrors() const {
    return mps::DriveError::get();
}

void Globals::IncDriveErrors() {
    mps::DriveError::increment();
}

void Globals::SetMotorsMode(bool stealth) {
    stealthMode = stealth;
    // @@TODO store into EEPROM
}

uint8_t Globals::StallGuardThreshold(config::Axis axis) const {
    // @@TODO this is not nice but we need some simple way of indexing the default SGTHRs
    static constexpr uint8_t defaultAxisSGTHRs[3] PROGMEM = { config::pulley.sg_thrs, config::selector.sg_thrs, config::idler.sg_thrs };
    return mps::AxisTMCSetup::get(axis, hal::progmem::read_byte(defaultAxisSGTHRs + axis));
}

void Globals::SetStallGuardThreshold(config::Axis axis, uint8_t sgthrs) {
    mps::AxisTMCSetup::set(axis, sgthrs); // store the value in EEPROM
}

} // namespace globals
} // namespace modules
