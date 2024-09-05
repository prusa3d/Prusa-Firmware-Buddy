#include "frame_qr_prompt.hpp"

#include <gui/frame_qr_layout.hpp>
#include <img_resources.hpp>
#include <guiconfig/wizard_config.hpp>

FrameQRPrompt::FrameQRPrompt(window_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &info_text, const char *qr_suffix)
    : window_frame_t(parent)
    , info(this, FrameQRLayout::text_rect(), is_multiline::yes, is_closed_on_click_t::no, info_text)
    , link(this, FrameQRLayout::link_rect(), is_multiline::no)
    , icon_phone(this, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72)
    , qr(this, FrameQRLayout::qrcode_rect(), Align_t::Center())
    , radio(this, WizardDefaults::RectRadioButton(0), fsm_phase) //
{
    const char *help_url = PrinterModelInfo::current().help_url;
    StringBuilder(link_buffer)
        .append_string("prusa.io/")
        .append_string(help_url)
        .append_char('-')
        .append_string(qr_suffix);
    link.SetText(string_view_utf8::MakeRAM(link_buffer.data()));

    qr.get_string_builder()
        .append_string("https://prusa.io/qr-")
        .append_string(help_url)
        .append_char('-')
        .append_string(qr_suffix);

    CaptureNormalWindow(radio);
    static_cast<window_frame_t *>(parent)->CaptureNormalWindow(*this);
}
