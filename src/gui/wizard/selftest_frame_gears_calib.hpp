#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class SelftestFrameGearsCalib : public AddSuperWindow<SelftestFrameNamedWithRadio> {
    FooterLine footer;
    window_wizard_progress_t progress;

    window_text_t text;
    window_text_t text_left;
    window_icon_t icon;

protected:
    virtual void change() override;

public:
    SelftestFrameGearsCalib(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
