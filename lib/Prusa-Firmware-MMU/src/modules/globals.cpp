/// @file globals.cpp
#include "globals.h"
#include "../config/config.h"
#include "permanent_storage.h"

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

void Globals::SetFilamentLoaded(uint8_t slot, FilamentLoadState newFilamentLoaded) {
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

} // namespace globals
} // namespace modules
