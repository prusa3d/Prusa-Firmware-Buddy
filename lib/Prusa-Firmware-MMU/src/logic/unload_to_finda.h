/// @file unload_to_finda.h
#pragma once
#include <stdint.h>

namespace logic {

/// @brief Unload to FINDA "small" state machine
///
/// "small" state machines will serve as building blocks for high-level commands/operations
/// - engage/disengage idler
/// - rotate pulley to some direction as long as the FINDA is on/off
/// - rotate some axis to some fixed direction
/// - load/unload to finda
struct UnloadToFinda {
    /// internal states of the state machine
    enum {
        EngagingIdler,
        UnloadingToFinda,
        WaitingForFINDA,
        OK,
        FailedFINDA,
        FailedFSensor
    };
    inline constexpr UnloadToFinda()
        : state(OK)
        , maxTries(3)
        , unloadStart_mm(0) {}

    /// Restart the automaton
    /// @param maxTries maximum number of retried attempts before reporting a fail
    void Reset(uint8_t maxTries);

    /// @returns true if the state machine finished its job, false otherwise
    bool Step();

    /// @returns internal state of the state machine
    inline uint8_t State() const { return state; }

private:
    uint8_t state;
    uint8_t maxTries;
    int32_t unloadStart_mm; // intentionally trying to avoid using U_mm because it is a float (reps. long double)
};

} // namespace logic
