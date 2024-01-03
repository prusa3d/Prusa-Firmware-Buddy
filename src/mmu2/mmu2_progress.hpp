#pragma once

#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_reporting.h"
#include "client_fsm_types.h"

namespace MMU2 {

/// This class keeps track of status reports from the MMU (and the UI/boilerplate code) and translates it to action progress (0-100) that is reported by the UI
class ProgressTrackingManager {

public:
    /// \return estimated progress percentage (0-100) of the currently running action
    inline uint8_t GetProgressPercentage() const {
        return progressPercentage_;
    }

    inline LoadUnloadMode GetLoadUnloadMode() const {
        return loadUnloadMode_;
    }

    inline RawProgressCode GetProgressCode() const {
        return progressCode_;
    }

    inline RawCommandInProgress GetCommandInProgress() const {
        return commandInProgress_;
    }

public:
    void ProcessReport(ProgressData d);

private:
    uint8_t progressPercentage_ = 0;
    RawProgressCode progressCode_ = static_cast<RawProgressCode>(ProgressCode::OK);
    RawCommandInProgress commandInProgress_ = CommandInProgress::NoCommand;
    LoadUnloadMode loadUnloadMode_ = LoadUnloadMode::Change;
};

} // namespace MMU2
