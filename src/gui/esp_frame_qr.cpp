#include "esp_frame_qr.hpp"

#include "selftest_esp_type.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "marlin_client.hpp"
#include "img_resources.hpp"
#include <cstring>

ESPFrameQR::ESPFrameQR(window_t *parent, PhasesESP ph, fsm::PhaseData data)
    : AddSuperWindow<ESPFrame>(parent, ph, data)
    , text(this, Positioner::textRect(), is_multiline::yes)
    , link(this, Positioner::linkRect(), is_multiline::no)
    , icon_phone(this, Positioner::phoneIconRect(), &img::hand_qr_59x72)
    , qr(this, Positioner::qrcodeRect(), QR_ADDR)

{
    text.SetAlignment(Align_t::LeftCenter());
    change();
}

void ESPFrameQR::change() {

    const char *txt = "MISSING";

    // texts
    switch (phase_current) {
    case PhasesESP::ESP_qr_instructions_flash:
        txt = N_("Use the online guide\nto setup your Wi-Fi");
        break;
    case PhasesESP::ESP_qr_instructions:
        txt = N_("To setup or troubleshoot your Wi-Fi, please visit:");
        break;
    default:
        break;
    }

    text.SetText(_(txt));
    link.SetText(string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(ADDR_IN_TEXT)));
};

constexpr Rect16 ESPFrameQR::Positioner::qrcodeRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 {
            GuiDefaults::ScreenWidth - WizardDefaults::MarginRight - qrcodeWidth,
            WizardDefaults::row_0,
            qrcodeWidth,
            qrcodeHeight
        };
    } else {
        return Rect16 { 160 - qrcodeWidth / 2, WizardDefaults::RectRadioButton(0).Top() - qrcodeHeight - 5, qrcodeWidth, qrcodeHeight };
    }
}

/** @returns Rect16 position and size of the phone icon widget */
constexpr Rect16 ESPFrameQR::Positioner::phoneIconRect() {
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
constexpr Rect16 ESPFrameQR::Positioner::textRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0, phoneIconRect().Left() - WizardDefaults::col_0, textHeight };
    } else {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0, WizardDefaults::X_space, textHeight };
    }
}

/** @returns Rect16 position and size of the link widget */
constexpr Rect16 ESPFrameQR::Positioner::linkRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::Y_space - textHeight, phoneIconRect().Left() - WizardDefaults::col_0, textHeight };
    } else {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0 + textHeight, WizardDefaults::X_space, WizardDefaults::txt_h };
    }
}
