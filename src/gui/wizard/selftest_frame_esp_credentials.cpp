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
    bool show_qr = false;
    ;

    //texts
    switch (phase_current) {
    case PhasesSelftest::ESP_credentials_instructions_flash:
        txt = N_("Use the online guide\nto setup your Wi-Fi\n" QR_ADDR_IN_TEXT);
        show_qr = true;
        break;
    case PhasesSelftest::ESP_credentials_instructions:
        txt = N_("Make sure USB drive with config file is connected.\n\nContinue to upload settings to printer.");
        break;
    case PhasesSelftest::ESP_credentials_instructions_qr:
        txt = N_("To setup or troubleshoot your Wi-Fi, please visit:\n" QR_ADDR_IN_TEXT);
        show_qr = true;
        break;
    case PhasesSelftest::ESP_credentials_USB_not_inserted:
        txt = N_("USB drive not detected! Insert USB drive first!");
        break;
    case PhasesSelftest::ESP_credentials_ask_gen:
        txt = N_("Generate Wi-Fi credentials?");
        break;
    case PhasesSelftest::ESP_credentials_ask_gen_overwrite:
        txt = N_("Config detected on the USB drive. Overwrite current file?");
        break;
    case PhasesSelftest::ESP_credentials_makefile_failed:
        txt = N_("Creating the file failed! Check the USB drive!");
        break;
    case PhasesSelftest::ESP_credentials_eject_USB:
        txt = N_("Success!\nRemove the drive.\nEdit the file in PC.");
        break;
    case PhasesSelftest::ESP_credentials_insert_USB:
        txt = N_("Insert USB drive with valid INI file.");
        break;
    case PhasesSelftest::ESP_credentials_invalid:
        txt = N_("Loading the file failed! Check the USB drive!");
        break;
    case PhasesSelftest::ESP_credentials_enabling_WIFI:
        txt = N_("Success!\nPlease wait until the connection is established.\n\nIf nothing happens after 5-8 minutes, repeat the process.");
        break;
    case PhasesSelftest::ESP_credentials_uploaded:
        txt = N_("Connection successfully established! Wi-Fi is now ready for use.");
        break;
    default:
        break;
    }

    if (show_qr) {
        qr.Show();
        icon_phone.Show();
    } else {
        qr.Hide();
        icon_phone.Hide();
    }

    text.SetText(_(txt));
};
