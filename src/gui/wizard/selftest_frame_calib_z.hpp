/**
 * @file selftest_frame_calib_z.hpp
 * @author Radek Vana
 * @brief part of screen containing Z calibration selftest
 * @date 2021-12-10
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class SelftestFrameCalibZ : public AddSuperWindow<SelftestFrameNamed> {
    window_wizard_progress_t progress; // used just to draw line

    window_icon_hourglass_t animation;

    window_text_t text_info;

protected:
    virtual void change() override;

public:
    SelftestFrameCalibZ(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
