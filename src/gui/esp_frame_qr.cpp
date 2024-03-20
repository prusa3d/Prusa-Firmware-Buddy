#include "esp_frame_qr.hpp"

#include <gui/frame_qr_layout.hpp>
#include "selftest_esp_type.hpp"
#include "i18n.h"
#include <guiconfig/wizard_config.hpp>
#include "marlin_client.hpp"
#include "img_resources.hpp"
#include <cstring>

ESPFrameQR::ESPFrameQR(window_t *parent, PhasesESP ph, fsm::PhaseData data)
    : AddSuperWindow<ESPFrame>(parent, ph, data)
    , text(this, FrameQRLayout::text_rect(), is_multiline::yes)
    , link(this, FrameQRLayout::link_rect(), is_multiline::no)
    , icon_phone(this, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72)
    , qr(this, FrameQRLayout::qrcode_rect(), QR_ADDR)

{
#if defined(USE_ILI9488)
    text.SetAlignment(Align_t::LeftCenter());
#endif
#if defined(USE_ST7789)
    link.SetAlignment(Align_t::CenterTop());
#endif
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
