/**
 * @file selftest_frame_esp.cpp
 */

#include "selftest_frame_esp.hpp"
#include "selftest_esp_type.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "marlin_client.hpp"
#include "resource.h"

static constexpr size_t icon_sz = 64;
static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::row_h * 2;
static constexpr size_t row_3 = row_2 + icon_sz;
static constexpr size_t col_0 = WizardDefaults::MarginLeft;

SelftestFrameESP::SelftestFrameESP(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data)
    , text_top(this, Rect16(col_0, WizardDefaults::row_1, WizardDefaults::X_space, WizardDefaults::row_h * 2), is_multiline::yes)
    , progress(this, row_2)
    , icon(this, Rect16((GuiDefaults::ScreenWidth - icon_sz) / 2, row_2, icon_sz, icon_sz), IDR_PNG_wifi_large_64x64)

    , text_bot(this, Rect16(col_0, row_3, WizardDefaults::X_space, WizardDefaults::row_h * 4), is_multiline::yes)

{
    progress.SetProgressPercent(0);
    text_top.SetAlignment(Align_t::LeftBottom());
    text_bot.SetAlignment(Align_t::LeftCenter());
    change();
}

void SelftestFrameESP::change() {
    SelftestESP_t dt(data_current);

    const char *txt_top = nullptr;
    const char *txt_bot = nullptr;
    bool show_icon = false;
    bool show_progress = false;

    //texts
    switch (phase_current) {
    case PhasesSelftest::ESP_ask_auto:
        txt_top = N_("WIFI module detected");
        txt_bot = N_("Firmware version 1.2.4 is available"); // TODO pass version
        show_icon = true;
        break;
    case PhasesSelftest::ESP_ask_from_menu:
        txt_top = N_("New version of WIFI firmware detected");
        txt_bot = N_("The device firmware needs to be updated. Firmware version 1.2.3 is available."); // TODO pass version
        show_icon = true;
        break;
    case PhasesSelftest::ESP_upload:
        txt_top = N_("WIFI update");
        txt_bot = N_("Do not disconnect WIFI module or turn power off");
        show_progress = true; // TODO pass progress
        break;
    case PhasesSelftest::ESP_passed:
        txt_top = N_("WIFI update");
        txt_bot = N_("Success");
        show_progress = true;
        progress.SetProgressPercent(100);
        break;
    case PhasesSelftest::ESP_failed:
        txt_top = N_("WIFI update");
        txt_bot = N_("Failed");
        show_progress = true;
        progress.SetProgressPercent(100);
        break;
    default:
        break;
    }

    if (txt_top) {
        text_top.Show();
        text_top.SetText(_(txt_top));
    } else {
        text_top.Hide();
    }

    if (txt_bot) {
        text_bot.Show();
        text_bot.SetText(_(txt_bot));
    } else {
        text_bot.Hide();
    }

    if (show_icon) {
        icon.Show();
    } else {
        icon.Hide();
    }

    if (show_progress) {
        progress.Show();
    } else {
        progress.Hide();
    }
};
