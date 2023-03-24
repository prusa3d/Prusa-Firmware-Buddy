/// @file cut_filament.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "unload_filament.h"
#include "feed_to_finda.h"

namespace logic {

/// @brief  A high-level command state machine - handles the complex logic of cutting filament
class CutFilament : public CommandBase {
public:
    inline CutFilament()
        : CommandBase() {}

    /// Restart the automaton
    /// @param param index of filament slot to perform cut onto
    void Reset(uint8_t param) override;

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override;

    ProgressCode State() const override;

    ErrorCode Error() const override;

private:
    constexpr static const uint16_t cutStepsPre = 700;
    constexpr static const uint16_t cutStepsPost = 150;
    UnloadFilament unl; ///< a high-level command/operation may be used as a building block of other operations as well
    FeedToFinda feed;
    uint8_t cutSlot;

    void SelectFilamentSlot();
};

/// The one and only instance of CutFilament state machine in the FW
extern CutFilament cutFilament;

} // namespace logic
