/// @file eject_filament.cpp
#include "eject_filament.h"
#include "../modules/buttons.h"
#include "../modules/finda.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/selector.h"
#include "../debug.h"

namespace logic {

EjectFilament ejectFilament;

void EjectFilament::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return;
    }

    error = ErrorCode::RUNNING;
    slot = param;

    if (mg::globals.FilamentLoaded() == mg::FilamentLoadState::NotLoaded) {
        state = ProgressCode::OK;
        dbg_logic_P(PSTR("Already ejected"));
    } else if (mg::globals.FilamentLoaded() >= mg::FilamentLoadState::AtPulley) {
        state = ProgressCode::UnloadingFilament;
        unl.Reset(param); //@@TODO probably act on active extruder only
    } else {
        MoveSelectorAside();
    }
}

void EjectFilament::MoveSelectorAside() {
    state = ProgressCode::ParkingSelector;
    const uint8_t selectorParkedPos = (slot <= 2) ? 4 : 0;
    mi::idler.Engage(slot);
    ms::selector.MoveToSlot(selectorParkedPos);
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
            state = ProgressCode::EjectingFilament;
            mm::motion.InitAxis(mm::Pulley);
            mm::motion.PlanMove<mm::Pulley>(-config::filamentMinLoadedToMMU, config::pulleySlowFeedrate);
        }
        break;
    case ProgressCode::EjectingFilament:
        if (mm::motion.QueueEmpty()) { // filament ejected
            state = ProgressCode::DisengagingIdler;
            mi::idler.Disengage();
        }
        break;
    case ProgressCode::DisengagingIdler:
        if (!mi::idler.Engaged()) { // idler disengaged
            mm::motion.Disable(mm::Pulley);
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::NotLoaded);
            state = ProgressCode::OK;
            error = ErrorCode::OK;
        }
        break;
    case ProgressCode::OK:
        dbg_logic_fP(PSTR("FilamentLoadState after Eject %d"), mg::globals.FilamentLoaded());
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
