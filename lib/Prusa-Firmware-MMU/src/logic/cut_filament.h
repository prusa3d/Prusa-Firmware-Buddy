/// @file cut_filament.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "unload_filament.h"
#include "feed_to_finda.h"
#include "retract_from_finda.h"

namespace logic {

/// @brief  A high-level command state machine - handles the complex logic of cutting filament
class CutFilament : public CommandBase {
public:
    inline constexpr CutFilament()
        : CommandBase()
        , cutSlot(0)
        , savedSelectorFeedRate_mm_s(0) {}

    /// Restart the automaton
    /// @param param index of filament slot to perform cut onto
    bool Reset(uint8_t param) override;

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override;

    ProgressCode State() const override;

    ErrorCode Error() const override;

private:
    constexpr static const uint16_t cutStepsPre = 700;
    constexpr static const uint16_t cutStepsPost = 150;
    UnloadFilament unl; ///< a high-level command/operation may be used as a building block of other operations as well
    FeedToFinda feed;
    RetractFromFinda retract;
    uint8_t cutSlot;
    uint16_t savedSelectorFeedRate_mm_s;

    void SelectFilamentSlot();
    void MoveSelector(uint8_t slot);
};

/// The one and only instance of CutFilament state machine in the FW
extern CutFilament cutFilament;

} // namespace logic
