/// @file tool_change.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "unload_filament.h"
#include "feed_to_finda.h"
#include "feed_to_bondtech.h"

namespace logic {

/// @brief  A high-level command state machine - handles the complex logic of tool change - which is basically a chain of an Unload and a Load operation.
class ToolChange : public CommandBase {
public:
    inline constexpr ToolChange()
        : CommandBase()
        , plannedSlot(-1) {}

    /// Restart the automaton
    /// @param param index of filament slot to change to - i.e. to load
    bool Reset(uint8_t param) override;

    /// @returns true if the state machine finished its job, false otherwise
    bool StepInner() override;

    ProgressCode State() const override;

    ErrorCode Error() const override;

#ifndef UNITTEST
private:
#endif
    void GoToFeedingToBondtech();
    void GoToFeedingToFinda();

    /// Common code for a correct completion of UnloadFilament
    void ToolChangeFinishedCorrectly();

    UnloadFilament unl; ///< a high-level command/operation may be used as a building block of other operations as well
    FeedToFinda feed;
    FeedToBondtech james; // bond ;)
    uint8_t plannedSlot;
};

/// The one and only instance of ToolChange state machine in the FW
extern ToolChange toolChange;

} // namespace logic
