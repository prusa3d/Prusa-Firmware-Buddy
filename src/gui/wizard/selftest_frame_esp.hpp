/**
 * @file selftest_frame_esp.hpp
 * @brief part of screen containing esp selftest (update)
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"

class SelftestFrameESP : public AddSuperWindow<SelftestFrameWithRadio> {
    char progr_text[sizeof("[0 / 0]")] = "[0 / 0]";
    window_text_t text_top;
    window_wizard_progress_t progress;
    window_text_t text_progress;
    window_icon_t icon;
    window_text_t text_bot;

protected:
    virtual void change() override;

public:
    SelftestFrameESP(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
