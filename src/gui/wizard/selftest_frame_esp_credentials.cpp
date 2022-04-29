/**
 * @file selftest_frame_esp_credentials.cpp
 */

#include "selftest_frame_esp_credentials.hpp"
#include "selftest_esp_type.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "marlin_client.hpp"
#include "resource.h"
#include <cstring>

SelftestFrameESP_credentials::QR::QR(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_qr_t>(parent, rect) {
    _(QR_ADDR).copyToRAM(text, MAX_LEN_4QR + 1);
}

static constexpr size_t icon_sz = 64;
static constexpr size_t col_0 = WizardDefaults::MarginLeft;

static constexpr size_t qr_size_px = 140;
static constexpr Rect16 qr_rect = { 160 - qr_size_px / 2, 200 - qr_size_px / 2, qr_size_px, qr_size_px }; /// center = [120,223]

SelftestFrameESP_credentials::SelftestFrameESP_credentials(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data)
    , text(this, Rect16(col_0, WizardDefaults::row_0, WizardDefaults::X_space, WizardDefaults::txt_h * 4), is_multiline::yes)
    , icon_phone(this, Rect16(20, 165, 64, 82), IDR_PNG_hand_qr)
    , qr(this, qr_rect)

{
    text.SetAlignment(Align_t::LeftCenter());
    change();
}

void SelftestFrameESP_credentials::change() {

    const char *txt = "MISSING";

    //texts
    switch (phase_current) {
    case PhasesSelftest::ESP_credentials_instructions:
        txt = N_("Wi-Fi credentials are not set.\nTo setup or troubleshoot your Wi-Fi, please visit:\n" QR_ADDR_IN_TEXT);
        break;
    case PhasesSelftest::ESP_credentials_instructions_stand_alone:
        txt = N_("To setup or troubleshoot your Wi-Fi, please visit:\n" QR_ADDR_IN_TEXT);
        break;
    case PhasesSelftest::ESP_credentials_ask_gen:
        txt = N_("Generate Wi-Fi credentials?");
        break;
    case PhasesSelftest::ESP_credentials_ask_gen_overwrite:
        txt = N_("Overwrite current file?");
        break;
    case PhasesSelftest::ESP_credentials_makefile_failed:
        txt = N_("File creation failed!");
        break;
    case PhasesSelftest::ESP_credentials_load:
        txt = N_("Load current file?");
        break;
    case PhasesSelftest::ESP_credentials_invalid:
        txt = N_("Wi-Fi credentials invalid!");
        break;
    case PhasesSelftest::ESP_credentials_uploaded:
        txt = N_("Wi-Fi credentials successfully updated.");
        break;
    default:
        break;
    }

    text.SetText(_(txt));
};
