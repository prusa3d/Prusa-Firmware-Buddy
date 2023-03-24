/// @file tool_change.cpp
#include "tool_change.h"
#include "../modules/buttons.h"
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

ToolChange toolChange;

void ToolChange::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return;
    }

    if (param == mg::globals.ActiveSlot() && (mg::globals.FilamentLoaded() == mg::FilamentLoadState::InNozzle)) {
        // we are already at the correct slot and the filament is loaded in the nozzle - nothing to do
        dbg_logic_P(PSTR("we are already at the correct slot and the filament is loaded - nothing to do\n"));
        return;
    }

    // we are either already at the correct slot, just the filament is not loaded - load the filament directly
    // or we are standing at another slot ...
    plannedSlot = param;

    if (mg::globals.FilamentLoaded() >= mg::FilamentLoadState::InSelector) {
        dbg_logic_P(PSTR("Filament is loaded --> unload"));
        state = ProgressCode::UnloadingFilament;
        unl.Reset(mg::globals.ActiveSlot());
    } else {
        state = ProgressCode::FeedingToFinda;
        error = ErrorCode::RUNNING;
        dbg_logic_P(PSTR("Filament is not loaded --> load"));
        mg::globals.SetFilamentLoaded(plannedSlot, mg::FilamentLoadState::InSelector);
        feed.Reset(true);
    }
}

void logic::ToolChange::GoToFeedingToBondtech() {
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::blink0, ml::off);
    james.Reset(3);
    state = ProgressCode::FeedingToBondtech;
    error = ErrorCode::RUNNING;
}

void logic::ToolChange::FinishedCorrectly() {
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::on, ml::off);
    state = ProgressCode::OK;
    error = ErrorCode::OK;
}

bool ToolChange::StepInner() {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        if (unl.StepInner()) {
            // unloading sequence finished - basically, no errors can occurr here
            // as UnloadFilament should handle all the possible error states on its own
            // There is no way the UnloadFilament to finish in an error state
            state = ProgressCode::FeedingToFinda;
            error = ErrorCode::RUNNING;
            mg::globals.SetFilamentLoaded(plannedSlot, mg::FilamentLoadState::AtPulley);
            feed.Reset(true);
        }
        break;
    case ProgressCode::FeedingToFinda:
        if (feed.Step()) {
            if (feed.State() == FeedToFinda::Failed) {
                GoToErrDisengagingIdler(ErrorCode::FINDA_DIDNT_SWITCH_ON); // signal loading error
            } else {
                GoToFeedingToBondtech();
            }
        }
        break;
    case ProgressCode::FeedingToBondtech:
        if (james.Step()) {
            if (james.State() == FeedToBondtech::Failed) {
                GoToErrDisengagingIdler(ErrorCode::FSENSOR_DIDNT_SWITCH_ON); // signal loading error
            } else {
                FinishedCorrectly();
            }
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
        case mui::Event::Left: // try to manually load just a tiny bit - help the filament with the pulley
            GoToErrEngagingIdler();
            break;
        case mui::Event::Middle: // try again the whole sequence
            Reset(mg::globals.ActiveSlot());
            break;
        case mui::Event::Right: // problem resolved - the user pushed the fillament by hand?
            // we should check the state of all the sensors and either report another error or confirm the correct state

            // First invalidate homing flags as the user may have moved the Idler or Selector accidentally
            InvalidateHoming();
            if (!mf::finda.Pressed()) {
                // FINDA is still NOT pressed - that smells bad
                error = ErrorCode::FINDA_DIDNT_SWITCH_ON;
                state = ProgressCode::ERRWaitingForUser; // stand still
            } else if (!mfs::fsensor.Pressed()) {
                // printer's filament sensor is still NOT pressed - that smells bad
                mg::globals.SetFilamentLoaded(plannedSlot, mg::FilamentLoadState::InSelector); // only assume the filament is in selector
                error = ErrorCode::FSENSOR_DIDNT_SWITCH_ON;
                state = ProgressCode::ERRWaitingForUser; // stand still - we may even try loading the filament into the nozzle
            } else {
                // all sensors are ok, we assume the user pushed the filament into the nozzle
                mg::globals.SetFilamentLoaded(plannedSlot, mg::FilamentLoadState::InNozzle);
                FinishedCorrectly();
            }
            break;
        default: // no event, continue waiting for user input
            break;
        }
        return false;
    }
    case ProgressCode::ERREngagingIdler:
        if (mi::idler.Engaged()) {
            state = ProgressCode::ERRHelpingFilament;
            mm::motion.PlanMove<mm::Pulley>(config::pulleyHelperMove, config::pulleySlowFeedrate);
        }
        return false;
    case ProgressCode::ERRHelpingFilament:
        // @@TODO helping filament needs improvement - the filament should try to move forward as long as the button is pressed
        if (mf::finda.Pressed()) {
            // the help was enough to press the FINDA, we are ok, continue normally
            GoToFeedingToBondtech();
        } else if (mfs::fsensor.Pressed()) {
            // the help was enough to press the filament sensor, we are ok, continue normally
            GoToFeedingToBondtech();
            // Beware, when the fsensor triggers, we still need to push the filament into the nozzle/gears
            // which requires restarting James from its last stage
            james.GoToPushToNozzle();
        } else if (mm::motion.QueueEmpty()) {
            // helped a bit, but FINDA/Fsensor didn't trigger, return to the main error state
            GoToErrDisengagingIdler(ErrorCode::FSENSOR_DIDNT_SWITCH_ON);
        }
        return false;
    default: // we got into an unhandled state, better report it
        state = ProgressCode::ERRInternal;
        error = ErrorCode::INTERNAL;
        return true;
    }
    return false;
}

ProgressCode ToolChange::State() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.State(); // report sub-automaton states properly
    default:
        return state;
    }
}

ErrorCode ToolChange::Error() const {
    switch (state) {
    case ProgressCode::UnloadingFilament: {
        ErrorCode ec = unl.Error(); // report sub-automaton errors properly, only filter out OK and replace them with RUNNING
        return ec == ErrorCode::OK ? ErrorCode::RUNNING : ec;
    }
    default:
        return error;
    }
}

} // namespace logic
