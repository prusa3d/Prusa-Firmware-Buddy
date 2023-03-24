/// @file globals.h
#pragma once
#include <stdint.h>

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

private:
    /// Sets the active slot, usually after some command/operation.
    /// Also updates the EEPROM records accordingly
    /// @param newActiveSlot the new slot index to set
    void SetActiveSlot(uint8_t newActiveSlot);

    uint8_t activeSlot;
    FilamentLoadState filamentLoaded;
    bool stealthMode;
};

/// The one and only instance of global state variables
extern Globals globals;

} // namespace globals
} // namespace modules

namespace mg = modules::globals;
