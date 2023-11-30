#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class SelftestFramePhaseStepping : public AddSuperWindow<SelftestFrameNamedWithRadio> {
    FooterLine footer;

    window_text_t text;

protected:
    virtual void change() override;

public:
    SelftestFramePhaseStepping(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
