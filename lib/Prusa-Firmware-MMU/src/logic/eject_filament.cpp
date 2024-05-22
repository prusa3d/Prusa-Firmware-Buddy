/// @file eject_filament.cpp
#include "eject_filament.h"
#include "../modules/buttons.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/pulley.h"
#include "../modules/selector.h"
#include "../modules/user_input.h"
#include "../debug.h"

namespace logic {

EjectFilament ejectFilament;

bool EjectFilament::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return false;
    }

    error = ErrorCode::RUNNING;
    slot = param;

    if (mg::globals.FilamentLoaded() >= mg::FilamentLoadState::AtPulley) {
        state = ProgressCode::UnloadingFilament;
        unl.Reset(param); //@@TODO probably act on active extruder only
    } else {
        MoveSelectorAside();
    }
    return true;
}

void EjectFilament::MoveSelectorAside() {
    state = ProgressCode::ParkingSelector;
    error = ErrorCode::RUNNING;
    const uint8_t selectorParkedPos = (slot <= 2) ? 4 : 0;
    if (ms::selector.MoveToSlot(selectorParkedPos) == ms::Selector::OperationResult::Refused) {
        GoToErrDisengagingIdler(ErrorCode::FINDA_FLICKERS);
    }
}

bool EjectFilament::StepInner() {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        if (unl.StepInner()) {
            // unloading sequence finished - basically, no errors can occur here
            // as UnloadFilament should handle all the possible error states on its own
            // There is no way the UnloadFilament to finish in an error state
            MoveSelectorAside();
        }
        break;
    case ProgressCode::ParkingSelector:
        if (mm::motion.QueueEmpty()) { // selector parked aside
            state = ProgressCode::EngagingIdler;
            mi::idler.Engage(slot);
        }
        break;
    case ProgressCode::EngagingIdler:
        if (mi::idler.Engaged()) {
            state = ProgressCode::EjectingFilament;
            mpu::pulley.InitAxis();
            mpu::pulley.PlanMove(config::ejectFromCuttingEdge, config::pulleySlowFeedrate);
        }
        break;
    case ProgressCode::EjectingFilament:
        if (mm::motion.QueueEmpty()) { // filament ejected
            GoToErrDisengagingIdler(ErrorCode::FILAMENT_EJECTED);
        }
        break;
    case ProgressCode::ERRDisengagingIdler:
        ErrDisengagingIdler();
        return false;
    case ProgressCode::ERRWaitingForUser: {
        // waiting for user buttons and/or a command from the printer
        mui::Event ev = mui::userInput.ConsumeEvent();
        switch (ev) {
        case mui::Event::Middle:
            ResumeIdlerSelector();
            switch (error) {
            case ErrorCode::FILAMENT_EJECTED: // the user clicked "Done", we can finish the Eject operation
                ml::leds.SetAllOff();
                FinishedOK();
                break;
            case ErrorCode::FINDA_FLICKERS:
                MoveSelectorAside();
                break;
            default:
                break;
            }
        default:
            break;
        }
        break;
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

ProgressCode EjectFilament::State() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.State(); // report sub-automaton states properly
    default:
        return state;
    }
}

ErrorCode EjectFilament::Error() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.Error(); // report sub-automaton errors properly
    default:
        return error;
    }
}

} // namespace logic
