#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class SelftestFramePhaseStepping : public AddSuperWindow<SelftestFrameWithRadio> {
    window_text_t text;
    window_text_t link;
    window_icon_t icon_phone;
    window_qr_t qr;
    std::array<char, 150> text_buffer;

    void flip_layout();

protected:
    virtual void change() override;

public:
    SelftestFramePhaseStepping(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
