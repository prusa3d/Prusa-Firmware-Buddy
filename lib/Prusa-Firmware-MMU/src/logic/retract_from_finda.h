/// @file retract_from_finda.h
#pragma once
#include <stdint.h>

namespace logic {

/// @brief Retract filament from FINDA to PTFE
///
/// Continuously pulls filament by a fixed length (originally 600 steps) + verifies FINDA is switched OFF while performing the move
/// Steps:
/// - engages idler (or makes sure the idler is engaged)
/// - pulls filament
/// - leaves idler engaged for chaining operations
struct RetractFromFinda {
    /// internal states of the state machine
    enum {
        EngagingIdler,
        UnloadBackToPTFE,
        OK,
        Failed
    };

    inline constexpr RetractFromFinda()
        : state(OK) {}

    /// Restart the automaton
    void Reset();

    /// @returns true if the state machine finished its job, false otherwise
    bool Step();

    /// This method may be used to check the result of the automaton
    /// @returns OK if everything went OK and FINDA triggered
    /// @returns Failed if the FINDA didn't trigger
    inline uint8_t State() const { return state; }

private:
    uint8_t state;
};

} // namespace logic
