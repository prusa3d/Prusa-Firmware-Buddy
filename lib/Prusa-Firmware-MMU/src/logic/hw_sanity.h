/// @file hw_sanity.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "../config/axis.h"
#include "../modules/leds.h"

namespace logic {

/// @brief Performs a sanity check of the hardware at reset/boot. Checks the following:
/// - TMC drivers using their IOIN registers (DIR/STEP/DRV_EN)
/// - ...
/// - Profit!

class HWSanity : public CommandBase {
public:
    inline constexpr HWSanity()
        : CommandBase() {}

    /// Restart the automaton
    bool Reset(uint8_t param) override;

    /// @returns true if the state machine finished its job, false otherwise.
    /// LED indicators during the test execution:
    /// Slots 1-3: Pin states for STEP, DIR, and ENA - G:  Set value matches readback, R: Readback disagrees.
    /// Slot 4: Axis under test - G: Idler, R: Selector, RG: Pully.
    /// Slot 5: G: Blinking to indicate test progression. R: Solid to indicate completed test w/ fault.
    /// Indicators at test end (fault condition):
    /// Slots 1-3 now indicate pins for Idler/Selector/Pulley, respectively:
    /// - Off: No faults detected.
    /// - G:   STEP fault
    /// - R:   DIR fault
    /// - RG:  EN fault.
    /// - Blinking R/G: Multiple fault, e.g both an EN fault together with STEP and/or DIR.
    /// Slot 4: Reserved
    /// Slot 5: R: Solid
    bool StepInner() override;

private:
    // Shared code fault display setup for each axis/slot
    static void SetFaultDisplay(uint8_t slot, uint8_t mask);

    // Prepares an axis for testing by initializing it and turning off the output.
    static void PrepareAxis(config::Axis axis);

    uint8_t test_step = 0;
    config::Axis axis = config::Axis::Pulley;
    uint8_t fault_masks[3] = { 0 };
    ProgressCode next_state = ProgressCode::HWTestBegin;
    uint16_t wait_start = 0;
    ml::Mode das_blinken_state = ml::off;
};

/// The one and only instance of hwSanity state machine in the FW
extern HWSanity hwSanity;

} // namespace logic
