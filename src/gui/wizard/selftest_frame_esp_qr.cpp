/**
 * @file selftest_frame_esp_qr.cpp
 */

#include "selftest_frame_esp_qr.hpp"
#include "selftest_esp_type.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "marlin_client.hpp"
#include "resource.h"
#include <cstring>

#define QR_ADDR "prusa.io/wifiminiqr"

static constexpr size_t icon_sz = 64;
static constexpr size_t col_0 = WizardDefaults::MarginLeft;

static constexpr size_t qr_size_px = 140;
static constexpr Rect16 qr_rect = { 160 - qr_size_px / 2, 200 - qr_size_px / 2, qr_size_px, qr_size_px }; /// center = [120,223]

SelftestFrameESP_qr::SelftestFrameESP_qr(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data)
    , text(this, Rect16(col_0, WizardDefaults::row_0, WizardDefaults::X_space, WizardDefaults::txt_h * 4), is_multiline::yes)
    , icon_phone(this, Rect16(20, 165, 64, 82), IDR_PNG_hand_qr)
    , qr(this, qr_rect, QR_ADDR)

{
    text.SetAlignment(Align_t::LeftCenter());
    change();
}

void SelftestFrameESP_qr::change() {

    const char *txt = "MISSING";

    //texts
    switch (phase_current) {
    case PhasesSelftest::ESP_qr_instructions_flash:
        txt = N_("Use the online guide\nto setup your Wi-Fi\nprusa.io/wifimini");
        break;
    case PhasesSelftest::ESP_qr_instructions:
        txt = N_("To setup or troubleshoot your Wi-Fi, please visit:\nprusa.io/wifimini");
        break;
    default:
        break;
    }

    text.SetText(_(txt));
};
