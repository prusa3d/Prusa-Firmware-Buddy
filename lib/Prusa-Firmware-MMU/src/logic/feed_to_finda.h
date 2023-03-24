/// @file feed_to_finda.h
#pragma once
#include <stdint.h>

namespace logic {

/// @brief Feed filament to FINDA
///
/// Continuously feed filament until FINDA is not switched ON
/// and than retract to align filament 600 steps away from FINDA.
/// Leaves the Idler engaged for chaining potential next operations.
/// Leaves the Pulley axis enabled for chaining potential next operations
struct FeedToFinda {
    /// internal states of the state machine
    enum {
        EngagingIdler,
        PushingFilament,
        UnloadBackToPTFE,
        DisengagingIdler,
        OK,
        Failed
    };

    inline FeedToFinda()
        : state(OK)
        , feedPhaseLimited(true) {}

    /// Restart the automaton
    /// @param feedPhaseLimited
    ///  * true feed phase is limited, doesn't react on button press
    ///  * false feed phase is unlimited, can be interrupted by any button press after blanking time
    ///  Beware: the function returns immediately without actually doing anything if the FINDA is "pressed", i.e. the filament is already at the FINDA
    void Reset(bool feedPhaseLimited);

    /// @returns true if the state machine finished its job, false otherwise
    bool Step();

    /// This method may be used to check the result of the automaton
    /// @returns OK if everything went OK and FINDA triggered (or the user pressed a button)
    /// @returns Failed if the FINDA didn't trigger
    inline uint8_t State() const { return state; }

private:
    uint8_t state;
    bool feedPhaseLimited;
};

} // namespace logic
