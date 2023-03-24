/// @file debouncer.h
#pragma once
#include <stdint.h>

namespace modules {

/// The debounce namespace provides a generic debouncing algorithm.
namespace debounce {

/// Implements debouncing on 2-state logic variables (true/false, high/low, on/off, pressed/unpressed)
/// Intentionally not modelled as a template to avoid code bloat
class Debouncer {
public:
    /// @param debounceTimeout initial debounce timeout in milliseconds @@TODO
    /// - after what time of having a pressed level the debouncer considers the level stable enough to report the Pressed state.
    inline constexpr Debouncer(uint8_t debounceTimeout)
        : timeLastChange(0)
        , debounceTimeout(debounceTimeout) {}

    /// @returns true if debounced value is currently considered as pressed
    inline bool Pressed() const { return f.state == State::WaitForRelease; }

    /// State machine stepping routine
    void Step(uint16_t time, bool press);

private:
    /// States of the debouncing automaton
    /// Intentionally not modeled as an enum class
    /// as it would impose additional casts which do not play well with the struct Flags
    /// and would make the code less readable
    enum State {
        Waiting = 0,
        Detected,
        WaitForRelease,
        Update
    };

    /// The sole purpose of this data struct is to save RAM by compressing several flags into one byte on the AVR
    struct Flags {
        uint8_t state : 2; ///< state of the debounced variable
        uint8_t tmp : 1; ///< temporary state of variable before the debouncing state machine finishes
        inline constexpr Flags()
            : state(State::Waiting)
            , tmp(false) {}
    };

    /// Flags and state of the debouncing automaton
    Flags f;

    /// Timestamp of the last change of raw input state for this variable
    uint16_t timeLastChange;
    uint8_t debounceTimeout;
};

} // namespace debounce
} // namespace modules

namespace md = modules::debounce;
