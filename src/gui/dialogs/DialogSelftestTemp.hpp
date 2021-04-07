#pragma once

#include "DialogStateful.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"

class DialogSelftestTemp : public IDialogMarlin {
    //noz
    window_text_t text_noz;
    window_wizard_progress_t progress_noz;
    window_text_t text_noz_prep;
    WindowIcon_OkNg icon_noz_prep;
    window_text_t text_noz_heat;
    WindowIcon_OkNg icon_noz_heat;
    //bed
    window_text_t text_bed;
    window_wizard_progress_t progress_bed;
    window_text_t text_bed_prep;
    WindowIcon_OkNg icon_bed_prep;
    window_text_t text_bed_heat;
    WindowIcon_OkNg icon_bed_heat;

protected:
    virtual bool change(uint8_t phase, fsm::PhaseData data) override;

public:
    DialogSelftestTemp();
};
