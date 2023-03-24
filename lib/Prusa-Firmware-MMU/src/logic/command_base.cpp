/// @file command_base.cpp
#include "command_base.h"
#include "../modules/globals.h"
#include "../modules/finda.h"
#include "../modules/fsensor.h"
#include "../modules/idler.h"
#include "../modules/selector.h"
#include "../modules/motion.h"
#include "../modules/leds.h"
#include "../modules/user_input.h"

namespace logic {

inline ErrorCode &operator|=(ErrorCode &a, ErrorCode b) {
    return a = (ErrorCode)((uint16_t)a | (uint16_t)b);
}

static ErrorCode TMC2130ToErrorCode(const hal::tmc2130::ErrorFlags &ef, uint8_t tmcIndex) {
    ErrorCode e = ErrorCode::RUNNING;

    if (ef.reset_flag) {
        e |= ErrorCode::TMC_RESET;
    }
    if (ef.uv_cp) {
        e |= ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP;
    }
    if (ef.s2g) {
        e |= ErrorCode::TMC_SHORT_TO_GROUND;
    }
    if (ef.otpw) {
        e |= ErrorCode::TMC_OVER_TEMPERATURE_WARN;
    }
    if (ef.ot) {
        e |= ErrorCode::TMC_OVER_TEMPERATURE_ERROR;
    }

    if (e != ErrorCode::RUNNING) {
        switch (tmcIndex) {
        case config::Axis::Pulley:
            e |= ErrorCode::TMC_PULLEY_BIT;
            break;
        case config::Axis::Selector:
            e |= ErrorCode::TMC_SELECTOR_BIT;
            break;
        case config::Axis::Idler:
            e |= ErrorCode::TMC_IDLER_BIT;
            break;
        default:
            break;
        }
    }

    return e;
}

bool CommandBase::Step() {
    ErrorCode tmcErr = ErrorCode::RUNNING;
    // check the global HW errors - may be we should avoid the modules layer and check for the HAL layer errors directly
    if (mi::idler.State() == mi::Idler::Failed) {
        state = ProgressCode::ERRTMCFailed;
        tmcErr |= TMC2130ToErrorCode(mi::idler.TMCErrorFlags(), mm::Axis::Idler);
    }
    if (ms::selector.State() == ms::Selector::Failed) {
        state = ProgressCode::ERRTMCFailed;
        tmcErr |= TMC2130ToErrorCode(ms::selector.TMCErrorFlags(), mm::Axis::Selector);
    }
    // may be we should model the Pulley as well...
    //    if (ms::selector.State() == ms::Selector::Failed) {
    //        state = ProgressCode::ERRTMCFailed;
    //        error |= TMC2130ToErrorCode(mm::motion.DriverForAxis(mm::Axis::Selector), mm::Axis::Selector);
    //        return true; // the HW error prevents us from continuing with the state machine - the MMU must be restarted/fixed before continuing
    //    }

    // @@TODO not sure how to prevent losing the previously accumulated error ... or do I really need to do it?
    // May be the TMC error word just gets updated with new flags as the motion proceeds
    // And how about the logical errors like FINDA_DIDNT_SWITCH_ON?
    if (tmcErr != ErrorCode::RUNNING) {
        error |= tmcErr;
        return true;
    }

    return StepInner();
}

void CommandBase::Panic(ErrorCode ec) {
    state = ProgressCode::ERRInternal;
    error = ec;
    for (uint8_t i = 0; i < config::toolCount; ++i) {
        ml::leds.SetMode(i, ml::green, ml::blink0);
        ml::leds.SetMode(i, ml::red, ml::blink0);
    }
}

void CommandBase::InvalidateHoming() {
    mi::idler.InvalidateHoming();
    ms::selector.InvalidateHoming();
}

void CommandBase::InvalidateHomingAndFilamentState() {
    InvalidateHoming();

    // reset the filament presence according to available sensor states
    bool fs = mfs::fsensor.Pressed();
    bool fi = mf::finda.Pressed();

    if (fs && fi) {
        mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::InNozzle);
    } else if (!fs && fi) {
        mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::InSelector);
    } else if (!fs && !fi) {
        mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::AtPulley);
    } else {
        // we can't say for sure - definitely an error in sensors or something is blocking them
        // let's assume there is a piece of filament present in the selector
        mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::InSelector);
    }
}

bool CommandBase::CheckToolIndex(uint8_t index) {
    if (index >= config::toolCount) {
        error = ErrorCode::INVALID_TOOL;
        return false;
    } else {
        error = ErrorCode::OK;
        return true;
    }
}

void CommandBase::ErrDisengagingIdler() {
    if (!mi::idler.Engaged()) {
        state = ProgressCode::ERRWaitingForUser;
        mm::motion.Disable(mm::Pulley);
        mui::userInput.Clear(); // remove all buffered events if any just before we wait for some input
    }
}

void CommandBase::GoToErrDisengagingIdler(ErrorCode ec) {
    state = ProgressCode::ERRDisengagingIdler;
    error = ec;
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::off, ml::blink0);
    mi::idler.Disengage();
}

void CommandBase::GoToErrEngagingIdler() {
    state = ProgressCode::ERREngagingIdler;
    error = ErrorCode::RUNNING;
    mi::idler.Engage(mg::globals.ActiveSlot());
}

} // namespace logic
