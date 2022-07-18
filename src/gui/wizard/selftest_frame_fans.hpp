/**
 * @file selftest_frame_fans.hpp
 * @author Radek Vana
 * @brief part of screen containing fan selftest
 * @date 2021-12-03
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"

class SelftestFrameFans : public AddSuperWindow<SelftestFrameNamed> {
    StatusFooter footer;
    window_wizard_progress_t progress;

    window_icon_t icon_hotend_fan;
    window_text_t text_hotend_fan;
    WindowIcon_OkNg icon_hotend_fan_state;

    window_icon_t icon_print_fan;
    window_text_t text_print_fan;
    WindowIcon_OkNg icon_print_fan_state;

    window_text_t text_info;

protected:
    virtual void change() override;

public:
    SelftestFrameFans(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
