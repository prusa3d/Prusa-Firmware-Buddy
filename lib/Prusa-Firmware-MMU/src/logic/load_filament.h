/// @file load_filament.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "feed_to_finda.h"
#include "retract_from_finda.h"

namespace logic {

/// @brief A high-level command state machine - handles the complex logic of loading filament into a filament slot.
class LoadFilament : public CommandBase {
public:
    inline constexpr LoadFilament()
        : CommandBase()
        , verifyLoadedFilament(0)
        , result(ResultCode::OK) {}

    /// Restart the automaton - performs unlimited rotation of the Pulley
    /// @param param index of filament slot to load
    bool Reset(uint8_t param) override;

    /// Restart the automaton for a limited rotation of the Pulley
    /// @param param index of filament slot to load
    void ResetLimited(uint8_t param);

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override;

    virtual ResultCode Result() const override { return result; }

private:
    void GoToRetractingFromFinda();
    void Reset2(bool feedPhaseLimited);

    /// Common code for a correct completion of UnloadFilament
    void LoadFinishedCorrectly();

    FeedToFinda feed;
    RetractFromFinda retract;

    /// As requested in MMU-116:
    /// Once the filament gets retracted after first feed, perform a short re-check by doing a limited load + retract.
    /// That ensures the filament can be loaded into the selector later when needed.
    /// verifyLoadedFilament holds the number of re-checks to be performed (we expect >1 re-checks will be requested one day ;) )
    uint8_t verifyLoadedFilament;

    /// Result of the LoadFilament command
    ResultCode result;
};

/// The one and only instance of LoadFilament state machine in the FW
extern LoadFilament loadFilament;

} // namespace logic
