/// @file cut_filament.cpp
#include "cut_filament.h"
#include "../modules/buttons.h"
#include "../modules/finda.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/pulley.h"
#include "../modules/selector.h"
#include "../modules/user_input.h"

namespace logic {

CutFilament cutFilament;

bool CutFilament::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return false;
    }

    error = ErrorCode::RUNNING;
    cutSlot = param;

    if (mg::globals.FilamentLoaded() >= mg::FilamentLoadState::InSelector) {
        state = ProgressCode::UnloadingFilament;
        unl.Reset(cutSlot);
    } else {
        SelectFilamentSlot();
    }
    return true;
}

void CutFilament::SelectFilamentSlot() {
    state = ProgressCode::SelectingFilamentSlot;
    mi::idler.Engage(cutSlot);
    MoveSelector(cutSlot);
    ml::leds.SetPairButOffOthers(cutSlot, ml::blink0, ml::off);
}

void CutFilament::MoveSelector(uint8_t slot) {
    if (ms::selector.MoveToSlot(slot) != ms::Selector::OperationResult::Accepted) {
        GoToErrDisengagingIdler(ErrorCode::FINDA_FLICKERS);
    }
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
            if (feed.Reset(true, true)) {
                state = ProgressCode::FeedingToFinda;
                error = ErrorCode::RUNNING;
            } else {
                // selector refused to move - FINDA problem suspected
                GoToErrDisengagingIdler(ErrorCode::FINDA_FLICKERS);
            }
        }
        break;
    case ProgressCode::FeedingToFinda:
        if (feed.Step()) {
            if (feed.State() == FeedToFinda::Failed) {
                GoToErrDisengagingIdler(ErrorCode::FINDA_DIDNT_SWITCH_ON); // signal loading error
            } else {
                // unload back to the pulley
                state = ProgressCode::RetractingFromFinda;
                retract.Reset();
            }
        }
        break;
    case ProgressCode::RetractingFromFinda:
        if (retract.Step()) {
            if (retract.State() == RetractFromFinda::Failed) {
                GoToErrDisengagingIdler(ErrorCode::FINDA_DIDNT_SWITCH_OFF); // signal loading error
            } else {
                // move selector aside - prepare the blade into active position
                state = ProgressCode::PreparingBlade;
                mg::globals.SetFilamentLoaded(cutSlot, mg::FilamentLoadState::AtPulley);
                ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::blink0, ml::off);
                MoveSelector(cutSlot + 1);
            }
        }
        break;
    case ProgressCode::PreparingBlade:
        if (ms::selector.Slot() == cutSlot + 1) {
            state = ProgressCode::PushingFilament;
            mpu::pulley.PlanMove(mg::globals.CutLength() + config::cuttingEdgeRetract, config::pulleySlowFeedrate);
        }
        break;
    case ProgressCode::PushingFilament:
        if (mm::motion.QueueEmpty()) {
            state = ProgressCode::DisengagingIdler;
            mi::idler.Disengage(); // disengage before doing a cut because we need extra power into the Selector motor
        }
        break;
    case ProgressCode::DisengagingIdler:
        if (mi::idler.Disengaged()) {
            state = ProgressCode::PerformingCut;
            // set highest available current for the Selector
            // Since we probably need to change the vSense bit (to double the current), we must reinit the axis
            mm::motion.InitAxis(mm::Selector, mm::MotorCurrents(mg::globals.CutIRunCurrent(), config::selector.iHold));
            // lower move speed
            savedSelectorFeedRate_mm_s = mg::globals.SelectorFeedrate_mm_s().v;
            mg::globals.SetSelectorFeedrate_mm_s(mg::globals.SelectorHomingFeedrate_mm_s().v);
            MoveSelector(cutSlot); // let it cut :)
        }
        break;
    case ProgressCode::PerformingCut:
        if (ms::selector.Slot() == cutSlot) { // this may not be necessary if we want the selector and pulley move at once
            state = ProgressCode::Homing;
            // revert current to Selector's normal value
            mm::motion.InitAxis(mm::Selector, mm::MotorCurrents(config::selector.iRun, config::selector.iHold));
            // revert move speed
            mg::globals.SetSelectorFeedrate_mm_s(savedSelectorFeedRate_mm_s);
            ms::selector.InvalidateHoming();
            mpu::pulley.PlanMove(-config::cuttingEdgeRetract, config::pulleySlowFeedrate);
        }
        break;
    case ProgressCode::Homing:
        if (ms::selector.HomingValid()) {
            state = ProgressCode::ReturningSelector;
        }
        break;
    case ProgressCode::ReturningSelector:
        if (ms::selector.State() == ms::selector.Ready) {
            FinishedOK();
            ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::on, ml::off);
        }
        break;
    case ProgressCode::OK:
        return true;
    case ProgressCode::ERRDisengagingIdler:
        ErrDisengagingIdler();
        return false;
    case ProgressCode::ERRWaitingForUser: {
        // waiting for user buttons and/or a command from the printer
        mui::Event ev = mui::userInput.ConsumeEvent();
        switch (ev) {
        case mui::Event::Middle: // try again the whole sequence
            InvalidateHoming();
            Reset(cutSlot);
            break;
        default: // no event, continue waiting for user input
            break;
        }
        return false;
    }
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
