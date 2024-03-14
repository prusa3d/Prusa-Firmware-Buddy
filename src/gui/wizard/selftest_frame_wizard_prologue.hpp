/**
 * @file selftest_frame_wizard_prologue.hpp
 * @brief part of screen containing wizard prologue information
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"

class SelftestFrameWizardPrologue : public AddSuperWindow<SelftestFrameWithRadio> {
    window_icon_t icon;
    window_text_t text_icon;

    window_text_t text_full_frame;

protected:
    virtual void change() override;

public:
    SelftestFrameWizardPrologue(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
