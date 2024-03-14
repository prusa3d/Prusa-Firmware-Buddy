/**
 * @file selftest_frame_axis.hpp
 * @author Radek Vana
 * @brief part of screen containing axis selftest
 * @date 2021-12-03
 */
#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class SelftestFrametAxis : public AddSuperWindow<SelftestFrameNamed> {
    FooterLine footer;
    window_wizard_progress_t progress;

    window_icon_t icon_x_axis;
    window_text_t text_x_axis;
    WindowIcon_OkNg icon_x_axis_state;

    window_icon_t icon_y_axis;
    window_text_t text_y_axis;
    WindowIcon_OkNg icon_y_axis_state;

    window_icon_t icon_z_axis;
    window_text_t text_z_axis;
    WindowIcon_OkNg icon_z_axis_state;

    window_text_t text_info;

protected:
    virtual void change() override;

public:
    SelftestFrametAxis(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
