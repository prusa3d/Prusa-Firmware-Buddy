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
#include "../modules/pulley.h"
#include "../modules/selector.h"
#include "../modules/user_input.h"
#include "../debug.h"

namespace logic {

ToolChange toolChange;

bool ToolChange::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return false;
    }

    if (param == mg::globals.ActiveSlot() && (mg::globals.FilamentLoaded() == mg::FilamentLoadState::InNozzle)) {
        // we are already at the correct slot and the filament is loaded in the nozzle - nothing to do
        dbg_logic_P(PSTR("we are already at the correct slot and the filament is loaded - nothing to do\n"));
        return true;
    }

    // @@TODO establish printer in charge of UI processing for the ToolChange command only.
    // We'll see how that works and then probably we'll introduce some kind of protocol settings to switch UI handling.
    mui::userInput.SetPrinterInCharge(true);

    // we are either already at the correct slot, just the filament is not loaded - load the filament directly
    // or we are standing at another slot ...
    plannedSlot = param;

    if (mg::globals.FilamentLoaded() >= mg::FilamentLoadState::InSelector) {
        dbg_logic_P(PSTR("Filament is loaded --> unload"));
        state = ProgressCode::UnloadingFilament;
        unl.Reset(mg::globals.ActiveSlot());
    } else {
        mg::globals.SetFilamentLoaded(plannedSlot, mg::FilamentLoadState::InSelector); // activate the correct slot, feed uses that
        if (feed.Reset(true, false)) {
            state = ProgressCode::FeedingToFinda;
            error = ErrorCode::RUNNING;
            dbg_logic_P(PSTR("Filament is not loaded --> load"));
        } else {
            // selector refused to move - FINDA problem suspected
            GoToErrDisengagingIdler(ErrorCode::FINDA_FLICKERS);
        }
    }
    return true;
}

void logic::ToolChange::GoToFeedingToBondtech() {
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::blink0, ml::off);
    james.Reset(3);
    state = ProgressCode::FeedingToBondtech;
    error = ErrorCode::RUNNING;
}

void logic::ToolChange::ToolChangeFinishedCorrectly() {
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::on, ml::off);
    mui::userInput.SetPrinterInCharge(false);
    FinishedOK();
}

void logic::ToolChange::GoToFeedingToFinda() {
    state = ProgressCode::FeedingToFinda;
    error = ErrorCode::RUNNING;
    mg::globals.SetFilamentLoaded(plannedSlot, mg::FilamentLoadState::AtPulley);
    if (!feed.Reset(true, false)) {
        GoToErrDisengagingIdler(ErrorCode::FINDA_FLICKERS);
    }
}

bool ToolChange::StepInner() {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        if (unl.StepInner()) {
            // unloading sequence finished - basically, no errors can occurr here
            // as UnloadFilament should handle all the possible error states on its own
            // There is no way the UnloadFilament to finish in an error state
            // But planning the next move can fail if Selector refuses moving to the next slot
            // - that scenario is handled inside GoToFeedingToFinda
            GoToFeedingToFinda();
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
            switch (james.State()) {
            case FeedToBondtech::Failed:
                GoToErrDisengagingIdler(ErrorCode::FSENSOR_DIDNT_SWITCH_ON); // signal loading error
                break;
            case FeedToBondtech::FSensorTooEarly:
                GoToErrDisengagingIdler(ErrorCode::FSENSOR_TOO_EARLY); // signal loading error
                break;
            default:
                ToolChangeFinishedCorrectly();
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
        case mui::Event::Middle: // try again the whole sequence
            // It looks like we don't have to reset the whole state machine but jump straight into the feeding phase.
            // The reasons are multiple:
            // - If an error happens during the unload phase, it is handled separately in the UnloadFilament state machine
            // - If an error happens during the feeding phase, the unload has been already successfully completed.
            //   And when restarted from the very beginning, the ToolChange does the last retract sequence from the UnloadFilament phase
            //   -> that is not healthy, because the filament gets pushed away from the Pulley and causes another error.
            // However - if we run into "FSensor didn't trigger", the situation is exactly opposite - it is beneficial
            // to unload the filament and try the whole sequence again
            // Therefore we only switch to FeedingToFinda if FINDA is not pressed (we suppose the filament is unloaded completely)
            //
            // MMU-191: if FSENSOR_DIDNT_SWITCH_ON was caused by misaligned Idler,
            // rehoming it may fix the issue when auto retrying -> no user intervention
            // So first invalidate homing flags as the user may have moved the Idler or Selector accidentally.
            //
            // Beware: we may run into issues when FINDA or FSensor do not work correctly. Selector may rely on the presumed filament position and actually cut it accidentally when trying to rehome.
            // It is yet to be seen if something like this can actually happen.
            InvalidateHoming();

            // It looks like we need to distinguish next steps based on what happened
            switch (error) {
            case ErrorCode::FSENSOR_TOO_EARLY:
                Reset(mg::globals.ActiveSlot());
                break;
            case ErrorCode::FINDA_FLICKERS:
                // This is a tricky part in case FINDA is flickering
                // If we already managed to finish the unload, we must assume finda should be NOT pressed.
                // If it is still pressed
                //  -> FINDA is flickering/badly tuned
                //  -> unreliable and the user didn't fix the issue
                //  -> we cannot do anything else but request the user to fix FINDA
                GoToFeedingToFinda();
                break;
            default:
                if (mf::finda.Pressed()) {
                    Reset(mg::globals.ActiveSlot());
                } else {
                    GoToFeedingToFinda();
                }
            }
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

ProgressCode ToolChange::State() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.State(); // report sub-automaton states properly
    case ProgressCode::FeedingToBondtech:
        // only process the important states
        switch (james.State()) {
        case FeedToBondtech::PushingFilamentToFSensor:
            return ProgressCode::FeedingToFSensor;
        case FeedToBondtech::PushingFilamentIntoNozzle:
            return ProgressCode::FeedingToNozzle;
        case FeedToBondtech::DisengagingIdler:
            return ProgressCode::DisengagingIdler;
        }
        [[fallthrough]]; // everything else is reported as is
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
