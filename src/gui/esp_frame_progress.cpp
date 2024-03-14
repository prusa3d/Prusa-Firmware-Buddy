#include "esp_frame_progress.hpp"

#include "selftest_esp_type.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "marlin_client.hpp"
#include "img_resources.hpp"

static constexpr size_t icon_sz = 64;
static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::row_h + WizardDefaults::txt_h;
static constexpr size_t row_2b = row_2 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + icon_sz;
static constexpr size_t col_0 = WizardDefaults::MarginLeft;

ESPFrameProgress::ESPFrameProgress(window_t *parent, PhasesESP ph, fsm::PhaseData data)
    : AddSuperWindow<ESPFrame>(parent, ph, data)
    , text_top(this, Rect16(col_0, WizardDefaults::row_0, WizardDefaults::X_space, WizardDefaults::txt_h * 2), is_multiline::yes)
    , progress(this, row_2)
    , text_progress(this, Rect16(col_0, row_2b, WizardDefaults::X_space, WizardDefaults::txt_h), is_multiline::no)
    , icon(this, Rect16((GuiDefaults::ScreenWidth - icon_sz) / 2, row_2, icon_sz, icon_sz), &img::wifi_64x64)

    , text_bot(this, Rect16(col_0, row_3, WizardDefaults::X_space, WizardDefaults::txt_h * 4), is_multiline::yes)

{
    progress.SetProgressPercent(0);
    text_top.SetAlignment(Align_t::LeftBottom());
    text_bot.SetAlignment(Align_t::LeftCenter());
    text_progress.SetAlignment(Align_t::Center());
    change();
}

void ESPFrameProgress::change() {
    SelftestESP_t dt(data_current);
    progress.SetProgressPercent(dt.progress);

    //"[0 / 0]";
    progr_text[1] = dt.current_file + '0';
    progr_text[5] = dt.count_of_files + '0';

    const char *txt_top = nullptr;
    const char *txt_bot = nullptr;
    bool show_icon = false;
    bool show_progress = false;

    /* Multiple variants of texts
     *   - For different display sizes
     *   - For different states
     * (* Let's see how well dead code elimination works - looks good so far!)
     */
    switch (phase_current) {
    case PhasesESP::ESP_progress_info:
        txt_top = GuiDefaults::ScreenWidth > 240 ? N_("Wi-Fi (ESP) module firmware updater.") : N_("Wi-Fi (ESP) module\nfirmware updater.");
        txt_bot = GuiDefaults::ScreenWidth > 240 ? N_("Continue to flash the ESP firmware.") : N_("Continue to flash\nthe ESP firmware.");
        show_icon = true;
        break;
    case PhasesESP::ESP_progress_upload:
        txt_top = N_("Updating Wi-Fi...");
        txt_bot = GuiDefaults::ScreenWidth > 240 ? N_("Do not unplug Wi-Fi or turn off printer!") : N_("Do not unplug Wi-Fi\nor turn off printer!");
        show_progress = true;
        break;
    case PhasesESP::ESP_progress_passed:
        txt_top = N_("Updating Wi-Fi...");
        txt_bot = GuiDefaults::ScreenWidth > 240 ? N_("Firmware flashing successful!") : N_("Firmware flashing\nsuccessful!");
        show_progress = true;
        break;
    case PhasesESP::ESP_progress_failed:
        txt_top = N_("Updating Wi-Fi...");
        txt_bot = GuiDefaults::ScreenWidth > 240 ? N_("Firmware flashing failed!") : N_("Firmware flashing\nfailed!");
        show_progress = true;
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
        text_progress.Show();
        text_progress.SetText(string_view_utf8::MakeRAM((uint8_t *)progr_text));
        text_progress.Invalidate(); // must invalidate, RAM text has same addres
    } else {
        progress.Hide();
        text_progress.Hide();
    }
};
