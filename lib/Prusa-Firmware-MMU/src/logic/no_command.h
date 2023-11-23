/// @file no_command.h
#pragma once
#include <stdint.h>
#include "command_base.h"

namespace logic {

/// @brief A dummy No-command operation just to make the init of the firmware consistent (and cleaner code during processing).
class NoCommand : public CommandBase {
public:
    inline constexpr NoCommand()
        : CommandBase() {}

    /// Restart the automaton
    bool Reset(uint8_t /*param*/) override { return true; }

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override { return true; }

    /// Used to report initialization errors (which can be reported if the UART started up).
    /// Intentionally only available in the "noCommand" operation
    /// which is only active when the MMU starts and before it gets any other command from the printer.
    inline void SetInitError(ErrorCode ec) { error = ec; }
};

/// The one and only instance of NoCommand state machine in the FW
extern NoCommand noCommand;

} // namespace logic
