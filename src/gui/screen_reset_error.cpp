// screen_reset_error.cpp

#include "screen_reset_error.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "version.h"
#include "support_utils.h"

ScreenResetError::ScreenResetError()
    : AddSuperWindow<screen_t>()
    , fw_version_txt(this, fw_version_rect, is_multiline::no)
    , signature_txt(this, signature_rect, is_multiline::no)
    , appendix_txt(this, appendix_rect, is_multiline::no)
    , sound_started(false) {

    ClrMenuTimeoutClose();
    ClrOnSerialClose();
    start_sound();

    fw_version_txt.set_font(resource_font(IDR_FNT_SMALL));
    signature_txt.set_font(resource_font(IDR_FNT_SMALL));
    appendix_txt.set_font(resource_font(IDR_FNT_SMALL));

    fw_version_txt.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::LeftTop() : Align_t::CenterTop());
    signature_txt.SetAlignment(Align_t::CenterTop());
    appendix_txt.SetAlignment(Align_t::CenterTop());

    /// fw version, hash, [fw signed], [appendix]
    static const constexpr uint16_t fw_version_str_len = 13 + 1; // combined max length of project_version + .._suffix_short + null
    static char fw_version[fw_version_str_len];                  // intentionally limited to the number of practically printable characters without overwriting the nearby hash text
                                                                 // snprintf will clamp the text if the input is too long
    snprintf(fw_version, sizeof(fw_version), "%s%s", project_version, project_version_suffix_short);
    fw_version_txt.SetText(_(fw_version));

    if (signature_exist()) {
        static const char signed_fw_str[] = "[S]";
        signature_txt.SetText(_(signed_fw_str));
    } else {
        signature_txt.Hide();
    }

    if (appendix_exist()) {
        static const char appendix_str[] = "[A]";
        appendix_txt.SetText(_(appendix_str));
    } else {
        appendix_txt.Hide();
    }
}

void ScreenResetError::start_sound() {
    if (!sound_started) {
        /// avoid collision of sounds
        Sound_Stop();
        Sound_Play(eSOUND_TYPE::CriticalAlert);
        sound_started = true;
    }
}

void ScreenResetError::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {
    case GUI_event_t::ENC_UP:
    case GUI_event_t::ENC_DN:
    case GUI_event_t::CLICK:
    case GUI_event_t::HOLD:
        Sound_Stop();
    default:
        break;
    }
}
