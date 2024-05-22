/// @file globals.h
#pragma once
#include <stdint.h>
#include "../config/config.h"

namespace modules {

/// The globals namespace provides all necessary facilities related to keeping track of global state of the firmware.
namespace globals {

/// Different stages of filament load.
/// Beware:
/// - the firmware depends on the order of these values to check for various situations.
/// - the unit tests abuse the bitmask-like values to check for multiple situations easily
enum FilamentLoadState : uint8_t {
    NotLoaded = 0, ///< not loaded in the MMU at all
    AtPulley = 1, ///< loaded to mmu (idler and pulley can grab it)
    InSelector = 2, ///< 'P1' loaded in selector (i.e. unsure where the filament is while doing some operation)
    InFSensor = 4, ///< 'f1' loaded into printer's filament sensor
    InNozzle = 8, ///< 'f2' loaded into printer's nozzle
};

static_assert(
    /*(FilamentLoadState::NotLoaded < FilamentLoadState::AtPulley)
&&*/
    (FilamentLoadState::AtPulley < FilamentLoadState::InSelector)
        && (FilamentLoadState::InSelector < FilamentLoadState::InFSensor)
        && (FilamentLoadState::InFSensor < FilamentLoadState::InNozzle),
    "incorrect order of Slot Filament Load States");

/// Globals keep track of global state variables in the firmware.
class Globals {
public:
    /// Initializes the global storage hive - basically looks into EEPROM to gather information.
    void Init();

    /// @returns active filament slot on the MMU unit. This value basically means there is some piece of filament blocking the selector from moving freely.
    /// Slots are numbered 0-4
    uint8_t ActiveSlot() const;

    /// @returns true if filament is considered as loaded
    FilamentLoadState FilamentLoaded() const;

    /// Sets the filament loaded flag value, usually after some command/operation.
    /// Also updates the EEPROM records accordingly
    /// @param slot slot index
    /// @param newFilamentLoaded new state
    void SetFilamentLoaded(uint8_t slot, FilamentLoadState newFilamentLoaded);

    /// @returns the total number of MMU errors so far
    /// Errors are stored in the EEPROM
    uint16_t DriveErrors() const;

    /// Increment MMU errors by 1
    void IncDriveErrors();

    /// Set normal or stealth mode for all of the motors
    /// Used to keep track of the last set mode to be able to return to it properly
    /// after homing sequences and/or after restarting the MMU
    /// @param stealth true means use stealth mode, false means use normal mode
    void SetMotorsMode(bool stealth);

    /// @returns true if the motors are to be operated in stealth mode
    bool MotorsStealth() const { return stealthMode; }

    config::U_mm FSensorToNozzle_mm() const { return config::U_mm({ (long double)fsensorToNozzle_mm }); }
    void ResetFSensorToNozzle() { fsensorToNozzle_mm = config::fsensorToNozzle.v; }
    void SetFSensorToNozzle_mm(uint8_t fss2Nozzle_mm) { fsensorToNozzle_mm = fss2Nozzle_mm; }

    config::U_mm FSensorUnloadCheck_mm() const { return config::U_mm({ (long double)fsensorUnloadCheck_mm }); }
    void ResetFSensorUnloadCheck() { fsensorUnloadCheck_mm = config::fsensorUnloadCheckDistance.v; }
    void SetFSensorUnloadCheck_mm(uint8_t fs2UnlCheck_mm) { fsensorUnloadCheck_mm = fs2UnlCheck_mm; }

    config::U_mm_s PulleyLoadFeedrate_mm_s() const { return config::U_mm_s({ (long double)pulleyLoadFeedrate_mm_s }); }
    void ResetPulleyLoadFeedrate() { pulleyLoadFeedrate_mm_s = config::pulleyLoadFeedrate.v; }
    void SetPulleyLoadFeedrate_mm_s(uint16_t pulleyLoadFR_mm_s) { pulleyLoadFeedrate_mm_s = pulleyLoadFR_mm_s; }

    config::U_mm_s PulleySlowFeedrate_mm_s() const { return config::U_mm_s({ (long double)pulleySlowFeedrate_mm_s }); }
    void ResetPulleySlowFeedrate() { pulleySlowFeedrate_mm_s = config::pulleySlowFeedrate.v; }
    void SetPulleySlowFeedrate_mm_s(uint16_t pulleySlowFR_mm_s) { pulleySlowFeedrate_mm_s = pulleySlowFR_mm_s; }

    config::U_mm_s PulleyUnloadFeedrate_mm_s() const { return config::U_mm_s({ (long double)pulleyUnloadFeedrate_mm_s }); }
    void ResetPulleyUnloadFeedrate() { pulleyUnloadFeedrate_mm_s = config::pulleyUnloadFeedrate.v; }
    void SetPulleyUnloadFeedrate_mm_s(uint16_t pulleyUnloadFR_mm_s) { pulleyUnloadFeedrate_mm_s = pulleyUnloadFR_mm_s; }

    config::U_mm_s SelectorFeedrate_mm_s() const { return config::U_mm_s({ (long double)selectorFeedrate_mm_s }); }
    void ResetSelectorFeedrate() { selectorFeedrate_mm_s = config::selectorFeedrate.v; }
    void SetSelectorFeedrate_mm_s(uint16_t selectorFR_mm_s) { selectorFeedrate_mm_s = selectorFR_mm_s; }

    config::U_deg_s IdlerFeedrate_deg_s() const { return config::U_deg_s({ (long double)idlerFeedrate_deg_s }); }
    void ResetIdlerFeedrate() { idlerFeedrate_deg_s = config::idlerFeedrate.v; }
    void SetIdlerFeedrate_deg_s(uint16_t idlerFR_deg_s) { idlerFeedrate_deg_s = idlerFR_deg_s; }

    config::U_mm_s SelectorHomingFeedrate_mm_s() const { return config::U_mm_s({ (long double)selectorHomingFeedrate_mm_s }); }
    void ResetSelectorHomingFeedrate() { selectorHomingFeedrate_mm_s = config::selectorHomingFeedrate.v; }
    void SetSelectorHomingFeedrate_mm_s(uint16_t selectorFR_mm_s) { selectorHomingFeedrate_mm_s = selectorFR_mm_s; }

    config::U_deg_s IdlerHomingFeedrate_deg_s() const { return config::U_deg_s({ (long double)idlerHomingFeedrate_deg_s }); }
    void ResetIdlerHomingFeedrate() { idlerHomingFeedrate_deg_s = config::idlerHomingFeedrate.v; }
    void SetIdlerHomingFeedrate_deg_s(uint16_t idlerFR_deg_s) { idlerHomingFeedrate_deg_s = idlerFR_deg_s; }

    /// @returns current StallGuard threshold for an axis
    uint8_t StallGuardThreshold(config::Axis axis) const;
    /// Stores the new StallGuard threshold for an axis into EEPROM (does not affect the current state of TMC drivers at all)
    void SetStallGuardThreshold(config::Axis axis, uint8_t sgthrs);

    /// @returns Cut iRun current level (value for TMC2130)
    uint8_t CutIRunCurrent() const { return cutIRunCurrent; }
    void ResetCutIRunCurrent() { cutIRunCurrent = config::selectorCutIRun; }
    void SetCutIRunCurrent(uint8_t v) { cutIRunCurrent = v; }

    config::U_mm CutLength() const { return config::U_mm({ (long double)cutLength_mm }); }
    void ResetCutLength() { cutLength_mm = config::cutLength.v; }
    void SetCutLength(uint8_t v) { cutLength_mm = v; }

private:
    /// Sets the active slot, usually after some command/operation.
    /// Also updates the EEPROM records accordingly
    /// @param newActiveSlot the new slot index to set
    void SetActiveSlot(uint8_t newActiveSlot);

    uint8_t activeSlot;
    FilamentLoadState filamentLoaded;
    bool stealthMode;
    uint8_t fsensorToNozzle_mm;
    uint8_t fsensorUnloadCheck_mm;

    uint16_t pulleyLoadFeedrate_mm_s;
    uint16_t pulleySlowFeedrate_mm_s;
    uint16_t pulleyUnloadFeedrate_mm_s;

    uint16_t selectorFeedrate_mm_s;
    uint16_t selectorHomingFeedrate_mm_s;

    uint16_t idlerFeedrate_deg_s;
    uint16_t idlerHomingFeedrate_deg_s;

    uint8_t cutIRunCurrent;
    uint8_t cutLength_mm;
};

/// The one and only instance of global state variables
extern Globals globals;

} // namespace globals
} // namespace modules

namespace mg = modules::globals;
