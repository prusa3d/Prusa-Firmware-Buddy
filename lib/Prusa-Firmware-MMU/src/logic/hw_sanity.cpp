/// @file hw_sanity.cpp
#include <string.h>
#include "hw_sanity.h"
#include "command_base.h"
#include "../modules/globals.h"
#include "../modules/motion.h"
#include "../modules/leds.h"
#include "../modules/timebase.h"

namespace logic {

// Copy-pasta from command_base, they're inline
// so this shouldn't affect code size.
inline ErrorCode &operator|=(ErrorCode &a, ErrorCode b) {
    return a = (ErrorCode)((uint16_t)a | (uint16_t)b);
}

using Axis = config::Axis;
using TMC2130 = hal::tmc2130::TMC2130;

static constexpr uint8_t LED_WAIT_MS = 50U;
static constexpr uint8_t TEST_PASSES = 3U;
static_assert(TEST_PASSES < 32); // Would overflow counters

HWSanity hwSanity;

bool HWSanity::Reset(uint8_t param) {
    state = ProgressCode::HWTestBegin;
    error = ErrorCode::RUNNING;
    axis = config::Axis::Idler;
    memset(fault_masks, 0, sizeof(fault_masks));
    return true;
}

enum pin_bits {
    BIT_STEP = 0b001,
    BIT_DIR = 0b010,
    BIT_ENA = 0b100,
};

void HWSanity::SetFaultDisplay(uint8_t slot, uint8_t mask) {
    ml::Mode red_mode = ml::off, green_mode = ml::off;
    if (mask & BIT_STEP) {
        green_mode = ml::on;
    }
    if (mask & BIT_DIR) {
        red_mode = ml::on;
    }
    if (mask & BIT_ENA) {
        green_mode = green_mode ? ml::blink0 : ml::on;
        red_mode = red_mode ? ml::blink0 : ml::on;
    }
    ml::leds.SetMode(slot, ml::green, green_mode);
    ml::leds.SetMode(slot, ml::red, red_mode);
}

void HWSanity::PrepareAxis(config::Axis axis) {
    mm::motion.InitAxis(axis);
    mm::motion.SetMode(axis, mm::Normal);
    // Clear TOFF so the motors don't actually step during the test.
    mm::motion.MMU_NEEDS_ATTENTION_DriverForAxis(axis).SetBridgeOutput(mm::axisParams[axis].params, false);
}

bool HWSanity::StepInner() {
    switch (state) {
    case ProgressCode::HWTestBegin:
        error = ErrorCode::RUNNING;
        test_step = 0;
        state = ProgressCode::HWTestIdler;
        break;
    case ProgressCode::HWTestIdler:
        axis = config::Axis::Idler;
        ml::leds.SetPairButOffOthers(3, ml::on, ml::off);
        state = ProgressCode::HWTestExec;
        next_state = ProgressCode::HWTestSelector;
        PrepareAxis(axis);
        break;
    case ProgressCode::HWTestSelector:
        axis = config::Axis::Selector;
        ml::leds.SetPairButOffOthers(3, ml::off, ml::on);
        state = ProgressCode::HWTestExec;
        next_state = ProgressCode::HWTestPulley;
        PrepareAxis(axis);
        break;
    case ProgressCode::HWTestPulley:
        axis = config::Axis::Pulley;
        ml::leds.SetPairButOffOthers(3, ml::on, ml::on);
        state = ProgressCode::HWTestExec;
        next_state = ProgressCode::HWTestCleanup;
        PrepareAxis(axis);
        break;
    // The main test loop for a given axis.
    case ProgressCode::HWTestDisplay:
        // Hold for a few ms while we display the last step result.
        if (!mt::timebase.Elapsed(wait_start, LED_WAIT_MS)) {
            break;
        } else {
            state = ProgressCode::HWTestExec;
            // display done, reset LEDs.
            ml::leds.SetAllOff();
        }
        [[fallthrough]];
    case ProgressCode::HWTestExec: {
        auto params = mm::axisParams[axis].params;
        auto &driver = mm::motion.MMU_NEEDS_ATTENTION_DriverForAxis(axis);
        if (test_step < (TEST_PASSES * 8)) // 8 combos per axis
        {
            uint8_t set_state = test_step % 8;
            // The order of the bits here is roughly the same as that of IOIN.
            driver.SetRawDir(params, set_state & BIT_DIR);
            driver.SetStep(params, set_state & BIT_STEP);
            driver.SetEnabled(params, set_state & BIT_ENA);
            uint32_t drv_ioin = driver.ReadRegister(params, hal::tmc2130::TMC2130::Registers::IOIN);
            // Compose IOIN to look like set_state.
            drv_ioin = (drv_ioin & 0b11) | ((drv_ioin & 0b10000) ? 0 : 4); // Note the logic inversion for ENA readback!
            uint8_t bit_errs = (drv_ioin ^ set_state);
            // Set the LEDs. Note RED is index 0 in the enum, so we want  the expression FALSE if there's an error.
            ml::leds.SetMode(0, static_cast<ml::Color>((bit_errs & BIT_STEP) == 0), ml::on);
            ml::leds.SetMode(1, static_cast<ml::Color>((bit_errs & BIT_DIR) == 0), ml::on);
            ml::leds.SetMode(2, static_cast<ml::Color>((bit_errs & BIT_ENA) == 0), ml::on);
            // Capture the error for later.
            fault_masks[axis] |= bit_errs;
            // Enter the wait state:
            wait_start = mt::timebase.Millis();
            das_blinken_state = das_blinken_state ? ml::off : ml::on;
            ml::leds.SetMode(4, ml::green, das_blinken_state);
            state = ProgressCode::HWTestDisplay;
            // Next iteration.
            test_step++;
        } else {
            // This pass is complete. Move on to the next motor or cleanup.
            driver.SetEnabled(params, false);
            driver.SetBridgeOutput(params, true);
            test_step = 0;
            state = next_state;
        }
    } break;
    case ProgressCode::HWTestCleanup:
        if (fault_masks[0] || fault_masks[1] || fault_masks[2]) {
            // error, display it and return the code.
            state = ProgressCode::ErrHwTestFailed;
            error = ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION;
            for (uint8_t axis = 0; axis < 3; axis++) {
                const uint8_t mask = fault_masks[axis];
                if (mask) {
                    error = logic::AddErrorAxisBit(error, axis);
                    SetFaultDisplay(axis, mask);
                }
            }
            ml::leds.SetMode(3, ml::red, ml::off);
            ml::leds.SetMode(3, ml::green, ml::off);
            ml::leds.SetMode(4, ml::red, ml::on);
            ml::leds.SetMode(4, ml::green, ml::off);
            return true;
        } else {
            ml::leds.SetAllOff();
            FinishedOK();
        }
    case ProgressCode::OK:
        return true;
    default: // we got into an unhandled state, better report it
        state = ProgressCode::ERRInternal;
        error = ErrorCode::INTERNAL;
        return true;
    }
    return false;
}

} // namespace logic
