/// @file command_base.cpp
#include "command_base.h"
#include "../application.h"
#include "../modules/globals.h"
#include "../modules/finda.h"
#include "../modules/fsensor.h"
#include "../modules/idler.h"
#include "../modules/pulley.h"
#include "../modules/selector.h"
#include "../modules/motion.h"
#include "../modules/leds.h"
#include "../modules/user_input.h"

namespace logic {

inline ErrorCode &operator|=(ErrorCode &a, ErrorCode b) {
    return a = (ErrorCode)((uint16_t)a | (uint16_t)b);
}

inline ErrorCode operator<<(ErrorCode a, uint8_t b) {
    return a = (ErrorCode)((uint16_t)a << b);
}

static ErrorCode TMC2130ToErrorCode(const hal::tmc2130::ErrorFlags &ef) {
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

    return e;
}

ErrorCode __attribute__((noinline)) AddErrorAxisBit(ErrorCode ec, uint8_t axis) {
    // From now on, we rely on specific ErrorCode bitmask values - that allowed some important optimizations.
    // In case someone fiddles with the bits, the build must fail.
    static_assert((uint16_t)ErrorCode::TMC_PULLEY_BIT == 0x0040);
    static_assert((uint16_t)ErrorCode::TMC_SELECTOR_BIT == 0x0080);
    static_assert((uint16_t)ErrorCode::TMC_IDLER_BIT == 0x0100);
    return ec |= (ErrorCode::TMC_PULLEY_BIT << axis);
}

ErrorCode CheckMovable(const mm::MovableBase &m) {
    switch (m.State() & (~mm::MovableBase::OnHold)) { // clear the on-hold bit from the state check
    case mm::MovableBase::TMCFailed:
        return AddErrorAxisBit(TMC2130ToErrorCode(m.TMCErrorFlags()), m.Axis());
    case mm::MovableBase::HomingFailed:
        return AddErrorAxisBit(ErrorCode::HOMING_FAILED, m.Axis());
    }
    return ErrorCode::RUNNING;
}

static inline ErrorCode WithoutAxisBits(ErrorCode ec) {
    return static_cast<ErrorCode>(
        static_cast<uint16_t>(ec)
        & (~(static_cast<uint16_t>(ErrorCode::TMC_SELECTOR_BIT)
            | static_cast<uint16_t>(ErrorCode::TMC_IDLER_BIT)
            | static_cast<uint16_t>(ErrorCode::TMC_PULLEY_BIT))));
}

bool CommandBase::WaitForOneModuleErrorRecovery(ErrorCode ec, modules::motion::MovableBase &m, uint8_t axisMask) {
    if (ec != ErrorCode::RUNNING) {
        if (stateBeforeModuleFailed == ProgressCode::Empty) {
            // a new problem with the movable modules
            // @@TODO not sure how to prevent losing the previously accumulated error ... or do I really need to do it?
            // May be the TMC error word just gets updated with new flags as the motion proceeds
            stateBeforeModuleFailed = state;
            errorBeforeModuleFailed = error;
            error = ec;
            state = ProgressCode::ERRWaitingForUser; // such a situation always requires user's attention -> let the printer display an error screen
            HoldIdlerSelector();
        }

        // are we already recovering an error - that would mean we got another one
        if (recoveringMovableErrorAxisMask) {
            error = ec;
            state = ProgressCode::ERRWaitingForUser; // such a situation always requires user's attention -> let the printer display an error screen
            HoldIdlerSelector();
        }

        switch (state) {
        case ProgressCode::ERRWaitingForUser: // waiting for a recovery - mask axis bits:
            if (WithoutAxisBits(ec) == ErrorCode::HOMING_FAILED) {
                // homing can be recovered
                mui::Event ev = mui::userInput.ConsumeEvent();
                if (ev == mui::Event::Middle) {
                    recoveringMovableErrorAxisMask |= axisMask;
                    ResumeIdlerSelector();
                    m.PlanHome(); // force initiate a new homing attempt
                    state = ProgressCode::Homing;
                    error = ErrorCode::RUNNING;
                }
            }
            // TMC errors cannot be recovered safely, waiting for power cycling the MMU
            return true;
        default:
            return true; // prevent descendant from taking over while in an error state
        }
    } else if (recoveringMovableErrorAxisMask & axisMask) {
        switch (state) {
        case ProgressCode::Homing:
            if (m.HomingValid()) {
                // managed to recover from a homing problem
                state = stateBeforeModuleFailed;
                error = errorBeforeModuleFailed;
                recoveringMovableErrorAxisMask &= (~axisMask);
                stateBeforeModuleFailed = ProgressCode::Empty;
                return false;
            }
            return true; // prevent descendant from taking over while recovering
        default:
            return false; // let descendant do its processing?
        }
    }
    return recoveringMovableErrorAxisMask & axisMask;
}

bool CommandBase::WaitForModulesErrorRecovery() {
    bool rv = WaitForOneModuleErrorRecovery(CheckMovable(mi::idler), mi::idler, 0x1);
    rv |= WaitForOneModuleErrorRecovery(CheckMovable(ms::selector), ms::selector, 0x2);
    rv |= WaitForOneModuleErrorRecovery(CheckMovable(mpu::pulley), mpu::pulley, 0x4);
    return rv;
}

bool CommandBase::Step() {
    if (WaitForModulesErrorRecovery() || state == ProgressCode::ERRInternal) {
        // ERRInternal: firmware panic was triggered
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
    ResumeIdlerSelector();
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

void CommandBase::HoldIdlerSelector() {
    mi::idler.HoldOn();
    ms::selector.HoldOn();
}

void CommandBase::ResumeIdlerSelector() {
    mi::idler.Resume();
    ms::selector.Resume();
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
    if (mi::idler.Disengaged()) {
        state = ProgressCode::ERRWaitingForUser;
        error = deferredErrorCode;
        deferredErrorCode = ErrorCode::OK; // and clear the deferredEC just for safety
        mg::globals.IncDriveErrors();
        mpu::pulley.Disable();
        mui::userInput.Clear(); // remove all buffered events if any just before we wait for some input
        HoldIdlerSelector(); // put all movables on hold until the error screen gets resolved
    }
}

void CommandBase::GoToErrDisengagingIdler(ErrorCode deferredEC) {
    state = ProgressCode::ERRDisengagingIdler;
    deferredErrorCode = deferredEC;
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::off, ml::blink0);
    mi::idler.Disengage();
}

void __attribute__((noinline)) CommandBase::GoToErrEngagingIdler() {
    state = ProgressCode::ERREngagingIdler;
    error = ErrorCode::RUNNING;
    mi::idler.Engage(mg::globals.ActiveSlot());
}

void __attribute__((noinline)) CommandBase::FinishedOK() {
    state = ProgressCode::OK;
    error = ErrorCode::OK;
    application.CommandFinishedCorrectly();
}

} // namespace logic
