/**
 * @file selftest_frame_calib_z.cpp
 * @author Radek Vana
 * @date 2021-12-10
 */

#include "selftest_frame_calib_z.hpp"
#include "i18n.h"
#include "fonts.hpp"
#include <guiconfig/wizard_config.hpp>

static constexpr size_t row_2 = 140;
static constexpr size_t row_3 = 200;

static constexpr const char *en_text_test_name = N_("Z-axis calibration");
static constexpr const char *en_text_info = N_("please wait");

SelftestFrameCalibZ::SelftestFrameCalibZ(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : SelftestFrameNamed(parent, ph, data, _(en_text_test_name))
    , progress(this, WizardDefaults::row_1)
    , animation(this, { int16_t(GuiDefaults::ScreenWidth / 2), int16_t(row_2) })
    , text_info(this, Rect16(WizardDefaults::col_0, row_3, WizardDefaults::X_space, WizardDefaults::row_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_info)) {
    progress.SetProgressPercent(100); // just draw orange line
    animation.SetRect(animation.GetRect() - Rect16::Left_t(animation.GetRect().Width() / 2)); // move to middle
    text_info.SetAlignment(Align_t::Center());

    change();
}

void SelftestFrameCalibZ::change() {};
