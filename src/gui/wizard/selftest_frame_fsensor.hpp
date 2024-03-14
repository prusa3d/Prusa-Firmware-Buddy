/**
 * @file selftest_frame_fsensor.hpp
 * @author Radek Vana
 * @brief part of screen containing sensor selftest
 * @date 2021-11-25
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class SelftestFrameFSensor : public AddSuperWindow<SelftestFrameNamedWithRadio> {
    FooterLine footer;
    window_wizard_progress_t progress;

    window_text_t text_left;
    window_text_t text_right;

    window_icon_t icon_left;
    window_icon_t icon_right;

    window_icon_hourglass_t animation;
    window_text_t text_animation; // animation test

    window_text_t text_result; // in middle of screen

protected:
    virtual void change() override;

public:
    SelftestFrameFSensor(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
