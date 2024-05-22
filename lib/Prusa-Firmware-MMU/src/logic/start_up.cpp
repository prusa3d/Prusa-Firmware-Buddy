/// @file
#include "start_up.h"
#include "../modules/buttons.h"
#include "../modules/finda.h"
#include "../modules/globals.h"
#include "../modules/user_input.h"

namespace logic {

StartUp startUp;

bool StartUp::Reset(uint8_t) {
    if (!CheckFINDAvsEEPROM()) {
        HoldIdlerSelector();
        SetInitError(ErrorCode::FINDA_VS_EEPROM_DISREPANCY);
    }

    return true;
}

bool StartUp::StepInner() {
    switch (state) {
    case ProgressCode::OK:
        return true;
    case ProgressCode::ERRWaitingForUser: {
        // waiting for user buttons and/or a command from the printer
        mui::Event ev = mui::userInput.ConsumeEvent();
        switch (ev) {
        case mui::Event::Middle:
            switch (error) {
            case ErrorCode::FINDA_VS_EEPROM_DISREPANCY:
                // Retry
                if (!CheckFINDAvsEEPROM()) {
                    error = ErrorCode::FINDA_VS_EEPROM_DISREPANCY;
                    state = ProgressCode::ERRWaitingForUser;
                } else {
                    ResumeIdlerSelector();
                    error = ErrorCode::OK;
                    state = ProgressCode::OK;
                }
                break;
            default:
                break;
            }
            break; // mui::Event::Middle
        default:
            break;
        }
        break; // ProgressCode::ERRWaitingForUser
    }
    default:
        // Do nothing
        break;
    }
    return false;
}

bool StartUp::CheckFINDAvsEEPROM() {
    bool ret = true;
    if (mf::finda.Pressed() && mg::globals.FilamentLoaded() < mg::InFSensor) {
        // This is a tricky situation - EEPROM doesn't have a record about loaded filament (blocking the Selector)
        // To be on the safe side, we have to override the EEPROM information about filament position - at least InFSensor
        // Moreover - we need to override the active slot position as well, because we cannot know where the Selector is.
        // For this we speculatively set the active slot to 2 (in the middle ;) )
        // Ideally this should be signalled as an error state and displayed on the printer and recovered properly.
        //mg::globals.SetFilamentLoaded(2, mg::InFSensor);
        ret = false;
    } else if (!mf::finda.Pressed() && mg::globals.FilamentLoaded() >= mg::InSelector) {
        // Opposite situation - not so dangerous, but definitely confusing to users.
        // FINDA is not pressed but we have a record in the EEPROM.
        // It has been decided, that we shall clear such a record from EEPROM automagically
        // and presume there is no filament at all (requires working FINDA)
        mg::globals.SetFilamentLoaded(config::toolCount, mg::AtPulley);
    }
    return ret;
}

} // namespace logic
