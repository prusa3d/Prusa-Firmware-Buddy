/**
 * @file selftest_frame_wizard_epilogue.cpp
 */

#include "selftest_frame_wizard_epilogue.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "resource.h"

static constexpr Rect16 getTextRect() {
    Rect16 ret = GuiDefaults::RectScreenNoHeader;
    Rect16::Top_t btn_top = GuiDefaults::GetButtonRect(ret).Top();
    ret = Rect16::Height_t(btn_top - ret.Top());
    ret = Rect16::Width_t(WizardDefaults::X_space);
    ret = Rect16::Left_t(WizardDefaults::MarginLeft);
    return ret;
}

static constexpr Rect16::Height_t icon_h = 150; // 140 + 10 padding

static constexpr Rect16 getIconRect() {
    Rect16 ret = getTextRect();
    ret = icon_h;
    return ret;
}

static constexpr Rect16 getTextIconRect() {
    Rect16 ret = getTextRect();
    ret -= icon_h;
    ret += Rect16::Top_t(icon_h);
    return ret;
}

SelftestFrameWizardEpilogue::SelftestFrameWizardEpilogue(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data)

    , icon(this, getIconRect(), IDR_PNG_pepa_140px)
    , text_icon(this, getTextIconRect(), is_multiline::yes) {

    icon.SetAlignment(Align_t::CenterTop());
    text_icon.SetAlignment(Align_t::CenterTop());

    change();
}

void SelftestFrameWizardEpilogue::change() {
    const char *txt_icon = nullptr;

    //texts
    switch (phase_current) {
    case PhasesSelftest::WizardEpilogue_ok:
        txt_icon = N_("Happy printing!");
        break;
    case PhasesSelftest::WizardEpilogue_nok:
        txt_icon = N_("The selftest failed to finish. Double-check the printer's wiring and axes. Then restart the Selftest.");
        break;
    default:
        break;
    }

    if (txt_icon) {
        text_icon.Show();
        text_icon.SetText(_(txt_icon));
    } else {
        text_icon.Hide();
    }
};
