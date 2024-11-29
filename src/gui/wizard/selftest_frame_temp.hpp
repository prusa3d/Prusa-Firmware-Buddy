#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"
#include <option/has_heatbreak_temp.h>

class ScreenSelftestTemp : public SelftestFrameWithRadio {
    FooterLine footer;

    window_frame_t test_frame;

    // noz
    window_text_t text_noz;
    window_wizard_progress_t progress_noz;
    window_text_t text_noz_prep;
    window_text_t text_noz_heat;

    // bed
    window_text_t text_bed;
    window_wizard_progress_t progress_bed;
    window_text_t text_bed_prep;
    WindowIcon_OkNg icon_bed_prep;
    window_text_t text_bed_heat;
    WindowIcon_OkNg icon_bed_heat;
#if HAS_HEATBREAK_TEMP()
    // heatbreak
    window_text_t text_heatbreak;
#endif

    window_text_t text_info;

    window_text_t text_dialog;

    // result per each HOTEND

    WindowIconOkNgArray icons_noz_prep;
    WindowIconOkNgArray icons_noz_heat;
#if HAS_HEATBREAK_TEMP()
    WindowIconOkNgArray icons_heatbreak;
#endif

protected:
    virtual void change() override;

public:
    ScreenSelftestTemp(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
