/// @file set_mode.h
#pragma once
#include <stdint.h>
#include "command_base.h"

namespace logic {

/// @brief Sets the mode of TMC2130 for all motors at once.
/// In the original proposal, the M0/M1 message was declared as a query, since it can be processed immediately.
/// The reality is a bit different - the TMC2130 driver cannot change from SpreadCycle into StealthMode while moving the motor,
/// at least not without serious jerking in most cases.
/// Therefore the M0/M1 messages were reconsidered into a command, because only one command at a time can be performed
/// (regardless of how long it takes it to finish) - that implies no motor moves are being performed while M0/M1 is being applied.
class SetMode : public CommandBase {
public:
    inline constexpr SetMode()
        : CommandBase() {}

    /// Restart the automaton
    bool Reset(uint8_t param) override;

    /// @returns true if the state machine finished its job, false otherwise
    /// Since we perform the TMC2130 mode change in the Reset directly, the return is always true here (command finished ok)
    bool StepInner() override { return true; }
};

/// The one and only instance of NoCommand state machine in the FW
extern SetMode setMode;

} // namespace logic
