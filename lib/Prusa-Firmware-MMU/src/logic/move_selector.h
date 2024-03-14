/// @file home.h
#pragma once
#include <stdint.h>
#include "command_base.h"

namespace logic {

/// @brief  A high-level command state machine - wrapps the rehoming procedure to be used from a printer
///
/// The Move-Selector operation consists of:
/// - issue a new coordinate to the Selector
///
/// Beware - Selector will NOT perform the move if filament sensor state is not in the right state
/// This high-level command is just a way to invoke moving the Selector from the printer
/// and/or from the MMU's buttons while all safety measures are kept.
class MoveSelector : public CommandBase {
public:
    inline constexpr MoveSelector()
        : CommandBase() {}

    /// Restart the automaton
    /// @param param target selector slot
    bool Reset(uint8_t param) override;

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override;
};

/// The one and only instance of  MoveSelector state machine in the FW
extern MoveSelector moveSelector;

} // namespace logic
