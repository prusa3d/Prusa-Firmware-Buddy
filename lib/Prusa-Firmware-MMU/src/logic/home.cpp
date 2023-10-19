/// @file home.cpp
#include "home.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/selector.h"
#include "../debug.h"

namespace logic {

Home home;

bool Home::Reset(uint8_t /*param*/) {
    error = ErrorCode::RUNNING;
    state = ProgressCode::Homing;
    InvalidateHomingAndFilamentState();
    return true;
}

bool Home::StepInner() {
    switch (state) {
    case ProgressCode::Homing:
        if (mi::idler.State() == mi::Idler::Ready && ms::selector.State() == ms::selector.Ready) {
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
