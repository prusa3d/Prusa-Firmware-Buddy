/**
 * @file selftest_frame_hot_end_sock.cpp
 */

#include "selftest_frame_hot_end_sock.hpp"
#include "selftest_hot_end_sock_type.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "img_resources.hpp"

static constexpr size_t margin_texts = WizardDefaults::MarginLeft;
static constexpr Align_t align_text_icon = Align_t::CenterTop();

static constexpr Rect16 get_text_rect() {
    Rect16 ret = GuiDefaults::RectScreenNoHeader;
    ret = Rect16::Height_t(WizardDefaults::row_h * 2 + WizardDefaults::txt_h);
    ret = Rect16::Width_t(GuiDefaults::ScreenWidth - 2 * margin_texts);
    ret = Rect16::Left_t(margin_texts);
    return ret;
}

static constexpr Rect16 get_text_sock_rect() {
    Rect16 ret = get_text_rect();
    ret = Rect16::Width_t(160);
    ret = Rect16::Height_t(WizardDefaults::txt_h);
    ret += Rect16::Top_t(WizardDefaults::row_h * 3);
    return ret;
}

static constexpr Rect16 get_text_nozzle_rect() {
    Rect16 ret = get_text_sock_rect();
    ret += Rect16::Top_t(WizardDefaults::row_h);
    return ret;
}

static constexpr Rect16 get_text_sock_value_rect() {
    Rect16 ret = get_text_rect();
    ret += Rect16::Left_t(ret.Width() - 160);
    ret = Rect16::Width_t(160);
    ret = Rect16::Height_t(WizardDefaults::txt_h);
    ret += Rect16::Top_t(WizardDefaults::row_h * 3);
    return ret;
}

static constexpr Rect16 get_text_nozzle_value_rect() {
    Rect16 ret = get_text_sock_value_rect();
    ret += Rect16::Top_t(WizardDefaults::row_h);
    return ret;
}

SelftestFrameHotEndSock::SelftestFrameHotEndSock(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data)
    , text(this, get_text_rect(), is_multiline::yes)
    , text_nozzle(this, get_text_nozzle_rect(), is_multiline::no, is_closed_on_click_t::no, _("Nozzle type"))
    , text_nozzle_value(this, get_text_nozzle_value_rect(), is_multiline::no)
    , text_sock(this, get_text_sock_rect(), is_multiline::no, is_closed_on_click_t::no, _("Hotend sock"))
    , text_sock_value(this, get_text_sock_value_rect(), is_multiline::no)

{
    text_nozzle_value.SetAlignment(Align_t::Right());
    text_sock_value.SetAlignment(Align_t::Right());
    change();
}

void SelftestFrameHotEndSock::change() {
    SelftestHotEndSockType result(data_current);

    const char *txt = nullptr;
    bool show_settings = false;

    // texts
    switch (phase_current) {
    case PhasesSelftest::SpecifyHotEnd:
        txt = N_("Attention, the test has failed. Check below the expected printer setup and adjust it accordingly:");
        show_settings = true;
        break;
    case PhasesSelftest::SpecifyHotEnd_sock:
        txt = N_("Do you have a silicone hotend sock installed?");
        break;
    case PhasesSelftest::SpecifyHotEnd_nozzle_type:
        txt = N_("What kind of nozzle type do you have installed?");
        break;
    case PhasesSelftest::SpecifyHotEnd_retry:
        txt = N_("Do you wish to retry the heater selftest?");
        break;
    default:
        break;
    }

    text.SetText(_(txt));

    if (show_settings) {
        // Disable showing of nozzle
        // text_nozzle.Show();
        // text_nozzle_value.Show();
        text_nozzle.Hide();
        text_nozzle_value.Hide();

        text_sock.Show();
        text_sock_value.Show();

        text_sock_value.SetText(result.has_sock ? _("Installed") : _("Not installed"));
        text_nozzle_value.SetText(result.prusa_stock_nozzle ? _("Stock Prusa") : _("High-flow"));
    } else {
        text_nozzle.Hide();
        text_nozzle_value.Hide();
        text_sock.Hide();
        text_sock_value.Hide();
    }
};
