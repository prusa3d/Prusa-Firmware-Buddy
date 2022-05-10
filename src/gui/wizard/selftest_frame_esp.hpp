/**
 * @file selftest_frame_esp.hpp
 * @brief part of screen containing update of esp credentials
 * this frame contains only one text box
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_wizard_progress.hpp"

class SelftestFrameESP : public AddSuperWindow<SelftestFrameWithRadio> {
    window_text_t text;

protected:
    virtual void change() override;

public:
    SelftestFrameESP(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
