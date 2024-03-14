/// @file load_filament.cpp
#include "load_filament.h"
#include "../modules/finda.h"
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

LoadFilament loadFilament;

bool LoadFilament::Reset(uint8_t param) {
    result = ResultCode::OK;

    if (!CheckToolIndex(param)) {
        return false;
    }
    if (mg::globals.FilamentLoaded() > mg::FilamentLoadState::InSelector) {
        return false; // avoid starting loadfilament if there is already some filament in the selector in some OTHER slot
    }
    // there is one special case though - same slot AND filament load state == InSelector (it MUST NOT be anywhere farther)
    if (mg::globals.FilamentLoaded() > mg::FilamentLoadState::AtPulley && mg::globals.ActiveSlot() != param) {
        return false;
    }
    dbg_logic_P(PSTR("Load Filament"));
    mg::globals.SetFilamentLoaded(param, mg::FilamentLoadState::AtPulley); // still at pulley, haven't moved yet
    verifyLoadedFilament = 1;
    Reset2(false);
    return true;
}

void LoadFilament::ResetLimited(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return;
    }
    dbg_logic_P(PSTR("Load Filament"));
    mg::globals.SetFilamentLoaded(param, mg::FilamentLoadState::AtPulley); // still at pulley, haven't moved yet
    Reset2(true);
}

void logic::LoadFilament::Reset2(bool feedPhaseLimited) {
    state = ProgressCode::FeedingToFinda;
    error = ErrorCode::RUNNING;
    if (!feed.Reset(feedPhaseLimited, true)) {
        // selector refused to move
        GoToErrDisengagingIdler(ErrorCode::FINDA_FLICKERS);
    } else {
        ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::blink0, ml::off);
    }
}

void logic::LoadFilament::GoToRetractingFromFinda() {
    state = ProgressCode::RetractingFromFinda;
    error = ErrorCode::RUNNING;
    retract.Reset();
}

void logic::LoadFilament::LoadFinishedCorrectly() {
    FinishedOK();
    ml::leds.SetAllOff();
    mpu::pulley.Disable();
}

bool LoadFilament::StepInner() {
    switch (state) {
    case ProgressCode::FeedingToFinda:
        if (feed.Step()) {
            switch (feed.State()) {
            case FeedToFinda::Failed: // @@TODO - try to repeat 6x - push/pull sequence - probably something to put into feed_to_finda as an option
                GoToErrDisengagingIdler(ErrorCode::FINDA_DIDNT_SWITCH_ON); // signal loading error
                break;
            case FeedToFinda::Stopped:
                // as requested in MMU-116 - stopping an unsuccessful feed should retract as well but not check the filament
                result = ResultCode::Cancelled;
                verifyLoadedFilament = 0;
                [[fallthrough]];
            case FeedToFinda::OK:
                GoToRetractingFromFinda();
                break;
            }
        }
        break;
    case ProgressCode::RetractingFromFinda:
        if (retract.Step()) {
            if (retract.State() == RetractFromFinda::Failed) {
                GoToErrDisengagingIdler(ErrorCode::FINDA_DIDNT_SWITCH_OFF); // signal loading error
            } else {
                if (verifyLoadedFilament) {
                    --verifyLoadedFilament;
                    // as requested in MMU-116 - once the filament gets retracted after first feed, perform a short re-check
                    // by doing a limited load + retract. That ensures the filament can be loaded into the selector later when needed.
                    ResetLimited(mg::globals.ActiveSlot());
                } else {
                    state = ProgressCode::DisengagingIdler;
                    mi::idler.Disengage();
                }
            }
        }
        break;
    case ProgressCode::DisengagingIdler:
        // beware - this state is being reused for error recovery
        // and if the selector decided to re-home, we have to wait for it as well
        // therefore: 'if (mi::idler.Disengaged())' : alone is not enough
        if (mi::idler.Disengaged() && ms::selector.Slot() == mg::globals.ActiveSlot()) {
            LoadFinishedCorrectly();
        }
        break;
    case ProgressCode::OK:
        return true;
    case ProgressCode::ERRDisengagingIdler: // couldn't load to FINDA
        ErrDisengagingIdler();
        return false;
    case ProgressCode::ERRWaitingForUser: {
        // waiting for user buttons and/or a command from the printer
        mui::Event ev = mui::userInput.ConsumeEvent();
        switch (ev) {
        case mui::Event::Middle: // try again the whole sequence
            ResumeIdlerSelector();
            // however it depends on the state of FINDA - if it is on, we must perform unload first
            if (!mf::finda.Pressed()) {
                Reset2(false);
            } else {
                GoToRetractingFromFinda();
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

} // namespace logic
