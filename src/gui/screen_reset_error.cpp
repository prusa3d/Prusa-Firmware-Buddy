// screen_reset_error.cpp

#include "screen_reset_error.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "version.h"
#include "support_utils.h"

ScreenResetError::ScreenResetError(const Rect16 &fw_version_rect)
    : AddSuperWindow<screen_t>()
    , fw_version_txt(this, fw_version_rect, is_multiline::no)
    , sound_started(false) {

    ClrMenuTimeoutClose();
    ClrOnSerialClose();
    start_sound();

    fw_version_txt.set_font(Font::small);

    fw_version_txt.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::LeftTop() : Align_t::CenterTop());

    const char *signed_str = "";
    if (signature_exist()) {
        static const char signed_fw_str[] = "[S]";
        signed_str = signed_fw_str;
    }

    const char *apendix_str = "";
    if (appendix_exist()) {
        static const char appendix_str[] = "[A]";
        apendix_str = appendix_str;
    }

    /// fw version full string [fw signed][appendix]
    static const constexpr uint16_t fw_version_str_len = 42;
    static char fw_version[fw_version_str_len];
    snprintf(fw_version, sizeof(fw_version), "%s %s%s", project_version_full, signed_str, apendix_str);
    fw_version_txt.SetText(_(fw_version));
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

void ScreenResetError::update_error_code([[maybe_unused]] uint16_t &error_code) {
#if PRINTER_IS_PRUSA_MK4
    if (!config_store().xy_motors_400_step.get()) {
        static_assert(ERR_PRINTER_CODE == 13, "PRUSA MK4's PID is no longer 13, which means this hardcoded calculation is no longer correct.");
        error_code += 8000; // If MK4 has 200 step motors, it means it is MK3.9, which has it's own product ID (21 instead of MK4's 13) - so 13XXX have to be change in runtime to 21XXX (+8000)
    }
#endif
}
