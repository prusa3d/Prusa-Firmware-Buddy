/// @file unload_filament.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "feed_to_finda.h"
#include "unload_to_finda.h"
#include "retract_from_finda.h"

namespace logic {

/// @brief A high-level command state machine - handles the complex logic of unloading filament
class UnloadFilament : public CommandBase {
public:
    inline constexpr UnloadFilament()
        : CommandBase() {}

    /// Restart the automaton
    /// @param param is not used, always unloads from the active slot
    bool Reset(uint8_t param) override;

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override;

private:
    constexpr static const uint8_t maxRetries = 1;

    /// Common code for a correct completion of UnloadFilament
    void UnloadFinishedCorrectly();
    void GoToRetractingFromFinda();
    void GoToRecheckFilamentAgainstFINDA();

    UnloadToFinda unl;
    FeedToFinda feed;
    RetractFromFinda retract;
};

/// The one and only instance of UnloadFilament state machine in the FW
extern UnloadFilament unloadFilament;

} // namespace logic
