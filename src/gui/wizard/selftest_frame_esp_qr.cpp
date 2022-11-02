/**
 * @file selftest_frame_esp_qr.cpp
 */

#include "selftest_frame_esp_qr.hpp"
#include "selftest_esp_type.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "marlin_client.hpp"
#include <cstring>

SelftestFrameESP_qr::SelftestFrameESP_qr(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data)
    , text(this, Positioner::textRect(), is_multiline::yes)
    , icon_phone(this, Positioner::phoneIconRect(), &png::hand_qr_59x72)
    , qr(this, Positioner::qrcodeRect(), QR_ADDR)

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

constexpr Rect16 SelftestFrameESP_qr::Positioner::qrcodeRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 {
            GuiDefaults::ScreenWidth - WizardDefaults::MarginRight - qrcodeWidth,
            WizardDefaults::row_0,
            qrcodeWidth,
            qrcodeHeight
        };
    } else {
        return Rect16 { 160 - qrcodeWidth / 2, 200 - qrcodeHeight / 2, qrcodeWidth, qrcodeHeight };
    }
}

/** @returns Rect16 position and size of the phone icon widget */
constexpr Rect16 SelftestFrameESP_qr::Positioner::phoneIconRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 {
            qrcodeRect().Left() - phoneWidth,
            (qrcodeRect().Top() + qrcodeRect().Bottom()) / 2 - phoneHeight / 2,
            phoneWidth,
            phoneHeight
        };
    } else {
        return Rect16 { 20, 165, phoneWidth, phoneHeight };
    }
}

/** @returns Rect16 position and size of the text widget */
constexpr Rect16 SelftestFrameESP_qr::Positioner::textRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::row_1, textWidth, textHeight };
    } else {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0, textWidth, textHeight };
    }
}
