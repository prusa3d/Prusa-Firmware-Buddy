#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class SelftestFrameNozzleDiameter : public AddSuperWindow<SelftestFrameNamedWithRadio> {
    FooterLine footer;

    window_text_t text_header;
    window_text_t text_details;

protected:
    void change() override;

public:
    SelftestFrameNozzleDiameter(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
    virtual ~SelftestFrameNozzleDiameter() = default;
};
