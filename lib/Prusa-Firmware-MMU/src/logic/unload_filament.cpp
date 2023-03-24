/// @file unload_filament.cpp
#include "unload_filament.h"
#include "../modules/finda.h"
#include "../modules/fsensor.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/selector.h"
#include "../modules/user_input.h"
#include "../debug.h"

namespace logic {

UnloadFilament unloadFilament;

void UnloadFilament::Reset(uint8_t /*param*/) {

    if (!mf::finda.Pressed() && mg::globals.FilamentLoaded() < mg::FilamentLoadState::InSelector) {
        // it looks like we have nothing in the PTFE tube, at least FINDA doesn't sense anything
        // so the filament has been probably already unloaded - terminate with OK or report an error?
        return;
    }

    // unloads filament from extruder - filament is above Bondtech gears
    mm::motion.InitAxis(mm::Pulley);
    state = ProgressCode::UnloadingToFinda;
    error = ErrorCode::RUNNING;
    unl.Reset(maxRetries);
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::off, ml::off);
}

void logic::UnloadFilament::FinishedCorrectly() {
    state = ProgressCode::OK;
    error = ErrorCode::OK;
    mm::motion.Disable(mm::Pulley);
    mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::AtPulley); // filament unloaded
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::off, ml::off);
}

bool UnloadFilament::StepInner() {
    switch (state) {
    // state 1 engage idler - will be done by the Unload to FINDA state machine
    case ProgressCode::UnloadingToFinda: // state 2 rotate pulley as long as the FINDA is on
        if (unl.Step()) {
            if (unl.State() == UnloadToFinda::Failed) {
                // couldn't unload to FINDA, report error and wait for user to resolve it
                GoToErrDisengagingIdler(ErrorCode::FINDA_DIDNT_SWITCH_OFF);
            } else if (mfs::fsensor.Pressed()) {
                // fsensor still pressed - that smells bad - a piece of filament may still be present in the heatsink
                // and that would cause serious problems while loading another filament
                GoToErrDisengagingIdler(ErrorCode::FSENSOR_DIDNT_SWITCH_OFF);
            } else {
                state = ProgressCode::RetractingFromFinda;
                retract.Reset();
            }
        }
        return false;
    case ProgressCode::RetractingFromFinda:
        if (retract.Step()) {
            if (retract.State() == RetractFromFinda::Failed) {
                GoToErrDisengagingIdler(ErrorCode::FINDA_DIDNT_SWITCH_OFF); // signal unloading error
            } else {
                state = ProgressCode::DisengagingIdler;
                mi::idler.Disengage();
            }
        }
        return false;
    case ProgressCode::DisengagingIdler:
        if (!mi::idler.Engaged()) {
            FinishedCorrectly();
        }
        return false;
    case ProgressCode::ERRDisengagingIdler: // couldn't unload to FINDA
        ErrDisengagingIdler();
        return false;
    case ProgressCode::ERRWaitingForUser: {
        // waiting for user buttons and/or a command from the printer
        mui::Event ev = mui::userInput.ConsumeEvent();
        switch (ev) {
        case mui::Event::Left: // try to manually unload just a tiny bit - help the filament with the pulley
            GoToErrEngagingIdler();
            break;
        case mui::Event::Middle: // try again the whole sequence
            if (mf::finda.Pressed()) {
                Reset(0);
            } else {
                state = ProgressCode::DisengagingIdler;
                mi::idler.Disengage();
            }
            break;
        case mui::Event::Right: // problem resolved - the user pulled the fillament by hand
            // we should check the state of all the sensors and either report another error or confirm the correct state

            // First invalidate homing flags as the user may have moved the Idler or Selector accidentally
            InvalidateHoming();
            if (mfs::fsensor.Pressed()) {
                // printer's filament sensor is still pressed - that smells bad
                error = ErrorCode::FSENSOR_DIDNT_SWITCH_OFF;
                state = ProgressCode::ERRWaitingForUser; // stand still
            } else if (mf::finda.Pressed()) {
                // FINDA is still pressed - that smells bad
                error = ErrorCode::FINDA_DIDNT_SWITCH_OFF;
                state = ProgressCode::ERRWaitingForUser; // stand still
            } else {
                // all sensors are ok
                FinishedCorrectly();
            }
            break;
        default:
            break;
        }
        return false;
    }
    case ProgressCode::ERREngagingIdler:
        if (mi::idler.Engaged()) {
            state = ProgressCode::ERRHelpingFilament;
            mm::motion.PlanMove<mm::Pulley>(-config::pulleyHelperMove, config::pulleySlowFeedrate);
        }
        return false;
    case ProgressCode::ERRHelpingFilament:
        if (!mf::finda.Pressed()) {
            // the help was enough to depress the FINDA, we are ok, continue normally
            state = ProgressCode::DisengagingIdler;
            error = ErrorCode::RUNNING;
        } else if (mm::motion.QueueEmpty()) {
            // helped a bit, but FINDA didn't trigger, return to the main error state
            GoToErrDisengagingIdler(ErrorCode::FINDA_DIDNT_SWITCH_OFF);
        }
        return false;
    case ProgressCode::OK:
        return true; // successfully finished
    default: // we got into an unhandled state, better report it
        state = ProgressCode::ERRInternal;
        error = ErrorCode::INTERNAL;
        return true;
    }
    return false;
}

} // namespace logic
