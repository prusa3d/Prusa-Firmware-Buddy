#include "mmu2_command_guard.h"

namespace MMU2 {
MMU2::CommandInProgressGuard::CommandInProgressGuard(CommandInProgress action, CommandInProgressManager &mgr)
    : CommandInProgressGuard(static_cast<RawCommandInProgress>(action), mgr) {}
MMU2::CommandInProgressGuard::CommandInProgressGuard(ExtendedCommandInProgress action, CommandInProgressManager &mgr)
    : CommandInProgressGuard(static_cast<RawCommandInProgress>(action), mgr) {}

CommandInProgressGuard::~CommandInProgressGuard() {
    decGuard(mgr);
}
CommandInProgressGuard::CommandInProgressGuard(uint8_t action, CommandInProgressManager &mgr)
    : mgr(mgr) {
    incGuard(action, mgr);
}

void CommandInProgressGuard::incGuard(uint8_t action, CommandInProgressManager &mgr) {
    if (mgr.activeGuardCount++ > 0) {
        return;
    }

    BeginReport(ProgressData(action, static_cast<RawProgressCode>(ProgressCode::EngagingIdler)));
    mgr.commandInProgress_ = action;
}
void CommandInProgressGuard::decGuard(CommandInProgressManager &mgr) {
    if (--mgr.activeGuardCount > 0) {
        return;
    }

    EndReport(ProgressData(mgr.commandInProgress_, static_cast<RawProgressCode>(ProgressCode::OK)));
    mgr.commandInProgress_ = static_cast<RawCommandInProgress>(CommandInProgress::NoCommand);
}

} // namespace MMU2
