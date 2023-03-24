/// @file cut_filament.cpp
#include "cut_filament.h"
#include "../modules/buttons.h"
#include "../modules/finda.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/selector.h"

namespace logic {

CutFilament cutFilament;

void CutFilament::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return;
    }

    error = ErrorCode::RUNNING;
    cutSlot = param;

    if (mg::globals.FilamentLoaded() >= mg::FilamentLoadState::InSelector) {
        state = ProgressCode::UnloadingFilament;
        unl.Reset(cutSlot);
    } else {
        SelectFilamentSlot();
    }
}

void CutFilament::SelectFilamentSlot() {
    state = ProgressCode::SelectingFilamentSlot;
    mi::idler.Engage(cutSlot);
    ms::selector.MoveToSlot(cutSlot);
    ml::leds.SetPairButOffOthers(cutSlot, ml::blink0, ml::off);
}

bool CutFilament::StepInner() {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        if (unl.StepInner()) {
            // unloading sequence finished - basically, no errors can occurr here
            // as UnloadFilament should handle all the possible error states on its own
            // There is no way the UnloadFilament to finish in an error state
            SelectFilamentSlot();
        }
        break;
    case ProgressCode::SelectingFilamentSlot:
        if (mi::idler.Engaged() && ms::selector.Slot() == cutSlot) { // idler and selector finished their moves
            mg::globals.SetFilamentLoaded(cutSlot, mg::FilamentLoadState::AtPulley);
            feed.Reset(true);
            state = ProgressCode::FeedingToFinda;
        }
        break;
    case ProgressCode::FeedingToFinda: // @@TODO this state will be reused for repeated cutting of filament ... probably there will be multiple attempts, not sure
        if (feed.Step()) {
            if (feed.State() == FeedToFinda::Failed) {
                // @@TODO
            } else {
                // unload back to the pulley
                state = ProgressCode::UnloadingToPulley;
                mm::motion.PlanMove<mm::Pulley>(-config::cutLength, config::pulleyFeedrate);
            }
        }
        break;
    case ProgressCode::UnloadingToPulley:
        if (mm::motion.QueueEmpty()) { // idler and selector finished their moves
            // move selector aside - prepare the blade into active position
            state = ProgressCode::PreparingBlade;
            mg::globals.SetFilamentLoaded(cutSlot, mg::FilamentLoadState::AtPulley);
            ms::selector.MoveToSlot(cutSlot + 1);
        }
    case ProgressCode::PreparingBlade:
        if (ms::selector.Slot() == cutSlot + 1) {
            state = ProgressCode::PushingFilament;
            mm::motion.PlanMove<mm::Pulley>(config::cutLength, config::pulleyFeedrate); //
        }
        break;
    case ProgressCode::PushingFilament:
        if (mm::motion.QueueEmpty()) {
            state = ProgressCode::PerformingCut;
            ms::selector.MoveToSlot(0);
        }
        break;
    case ProgressCode::PerformingCut:
        if (ms::selector.Slot() == 0) { // this may not be necessary if we want the selector and pulley move at once
            state = ProgressCode::ReturningSelector;
            ms::selector.MoveToSlot(5); // move selector to the other end of its axis to cleanup
        }
        break;
    case ProgressCode::ReturningSelector:
        if (ms::selector.Slot() == 5) { // selector returned to position, feed the filament back to FINDA
            state = ProgressCode::OK;
            error = ErrorCode::OK;
            ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::on, ml::off);
            feed.Reset(true);
        }
        break;
    case ProgressCode::OK:
        return true;
    default: // we got into an unhandled state, better report it
        state = ProgressCode::ERRInternal;
        error = ErrorCode::INTERNAL;
        return true;
    }
    return false;
}

ProgressCode CutFilament::State() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.State(); // report sub-automaton states properly
    default:
        return state;
    }
}

ErrorCode CutFilament::Error() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.Error(); // report sub-automaton errors properly
    default:
        return error;
    }
}

} // namespace logic
