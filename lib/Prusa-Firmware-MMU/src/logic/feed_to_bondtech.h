/// @file feed_to_bondtech.h
#pragma once
#include <stdint.h>
#include "../modules/axisunit.h"

namespace logic {

/// @brief Feed filament to Bondtech gears of the printer
///
/// Continuously feed filament until the printer detects the filament in its filament sensor.
/// Then it feeds a bit more very gently to push the filament into the nozzle
/// Disengages the Idler after finishing the feed.
/// Disables the Pulley axis after disengaging the idler.
///
/// If filament has been successfully fed into the fsensor,
/// records/updates PTFE length.
/// To prevent constant EEPROM updates only significant changes are recorded.
struct FeedToBondtech {
    /// internal states of the state machine
    enum {
        EngagingIdler,
        PushingFilamentFast,
        PushingFilamentToFSensor,
        PushingFilamentIntoNozzle,
        PartiallyDisengagingIdler,
        DisengagingIdler,
        OK,
        Failed,
        FSensorTooEarly,
        //        PulleyStalled
    };

    inline constexpr FeedToBondtech()
        : state(OK)
        , maxRetries(1) {}

    /// Restart the automaton
    void Reset(uint8_t maxRetries);

    /// @returns true if the state machine finished its job, false otherwise
    bool Step();

    /// This method may be used to check the result of the automaton
    /// @returns OK if everything went OK and printer's filament sensor triggered
    /// @returns Failed if the maximum feed length has been reached without the the printer's fsensor trigger being reported
    inline uint8_t State() const { return state; }

    /// Transition to PushingFilamentIntoNozzle.
    /// Allows jumping into the state machine from outside just to push filament slowly into the nozzle
    /// (this principle uses tool change)
    void GoToPushToNozzle();

private:
    uint8_t state;
    uint8_t maxRetries;
};

} // namespace logic
