#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class ScreenSelftestTemp : public AddSuperWindow<SelftestFrame> {
    StatusFooter footer;
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
    virtual void change() override;

public:
    ScreenSelftestTemp(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
