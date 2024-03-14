/**
 * @file selftest_frame_wizard_epilogue.hpp
 * @brief part of screen containing wizard epilogue information
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"

class SelftestFrameWizardEpilogue : public AddSuperWindow<SelftestFrameWithRadio> {
    window_icon_t icon;
    window_text_t text_icon;

protected:
    virtual void change() override;

public:
    SelftestFrameWizardEpilogue(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
