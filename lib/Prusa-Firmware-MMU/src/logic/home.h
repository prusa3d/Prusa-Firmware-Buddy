/// @file home.h
#pragma once
#include <stdint.h>
#include "command_base.h"

namespace logic {

/// @brief  A high-level command state machine - wrapps the rehoming procedure to be used from a printer
///
/// The home operation consists of:
/// - invalidates Idler's and Selector's homing flags
/// - waits until Idler and Selector finish their homing sequences
///
/// Beware - Idler and Selector will NOT perform the homing moves if filament sensor state is not in the right state
/// - Idler: filament must not be in the fsensor or nozzle
/// - Selector: filament must not be in the selector, fsensor or nozzle
/// It is up to the printer to let the MMU unload filament first (to make sure everything is safe) and then issue the homing command
///
/// Moreover - Idler and Selector try to home automagically a runtime when they know it is safe.
/// This high-level command is just a way to invoke re-homing from the printer while all safety measures are kept.
class Home : public CommandBase {
public:
    inline constexpr Home()
        : CommandBase() {}

    /// Restart the automaton
    bool Reset(uint8_t /*param*/) override;

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override;
};

/// The one and only instance of Home state machine in the FW
extern Home home;

} // namespace logic
