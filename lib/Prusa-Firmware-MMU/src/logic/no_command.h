/// @file no_command.h
#pragma once
#include <stdint.h>
#include "command_base.h"

namespace logic {

/// @brief A dummy No-command operation just to make the init of the firmware consistent (and cleaner code during processing).
class NoCommand : public CommandBase {
public:
    inline NoCommand()
        : CommandBase() {}

    /// Restart the automaton
    void Reset(uint8_t /*param*/) override {}

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override { return true; }
};

/// The one and only instance of NoCommand state machine in the FW
extern NoCommand noCommand;

} // namespace logic
