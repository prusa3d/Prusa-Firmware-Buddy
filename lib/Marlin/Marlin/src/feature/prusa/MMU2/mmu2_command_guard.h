#pragma once

#include "mmu2_reporting.h"

namespace MMU2 {

class CommandInProgressManager {
    friend class CommandInProgressGuard;

public:
    inline bool isCommandInProgress() const {
        return activeGuardCount > 0;
    }

    inline RawCommandInProgress commandInProgress() const {
        return commandInProgress_;
    }

private:
    /// Current top level command in progress being tracked
    RawCommandInProgress commandInProgress_ = static_cast<RawCommandInProgress>(CommandInProgress::NoCommand);

    /// Number of action guard instances
    uint8_t activeGuardCount = 0;
};

/// Scope guard all MMU actions should be covered around
class CommandInProgressGuard {

public:
    CommandInProgressGuard(CommandInProgress action, CommandInProgressManager &mgr);
    CommandInProgressGuard(ExtendedCommandInProgress action, CommandInProgressManager &mgr);

    ~CommandInProgressGuard();

    static void incGuard(uint8_t action, CommandInProgressManager &mgr);
    static void decGuard(CommandInProgressManager &mgr);

private:
    CommandInProgressGuard(uint8_t action, CommandInProgressManager &mgr);

private:
    CommandInProgressManager &mgr;
};

} // namespace MMU2
