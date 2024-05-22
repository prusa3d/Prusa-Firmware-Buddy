/// @file no_command.h
#pragma once
#include <stdint.h>
#include "command_base.h"

namespace logic {

/// @brief Firmware start up sequence with error handling & reporting
class StartUp : public CommandBase {
public:
    inline constexpr StartUp()
        : CommandBase() {}

    /// Restart the automaton
    bool Reset(uint8_t /*param*/) override;

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override;

    /// Used to report initialization errors (which can be reported if the UART started up).
    /// Intentionally only available in the "noCommand" operation
    /// which is only active when the MMU starts and before it gets any other command from the printer.
    inline void SetInitError(ErrorCode ec) {
        error = ec;
        state = ProgressCode::ERRWaitingForUser;
    }

private:
    /// @returns true if there is no discrepency, false otherwise
    static bool CheckFINDAvsEEPROM();
};

/// The one and only instance of StartUp state machine in the FW
extern StartUp startUp;

} // namespace logic
