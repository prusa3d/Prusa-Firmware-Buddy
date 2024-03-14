/// @file
#include "move_selector.h"
#include "../modules/globals.h"
#include "../modules/selector.h"
#include "../debug.h"

namespace logic {

MoveSelector moveSelector;

bool MoveSelector::Reset(uint8_t param) {
    state = ProgressCode::MovingSelector;

    if (ms::selector.MoveToSlot(param) != ms::Selector::OperationResult::Refused) {
        // operation accepted
        error = ErrorCode::RUNNING;
        return true;
    } else {
        error = ErrorCode::HOMING_SELECTOR_FAILED; // @@TODO
        return false;
    }
}

bool MoveSelector::StepInner() {
    switch (state) {
    case ProgressCode::MovingSelector:
        if (ms::selector.State() == ms::selector.Ready) {
            mg::globals.SetFilamentLoaded(ms::selector.Slot(), mg::FilamentLoadState::AtPulley);
            FinishedOK();
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

} // namespace logic
