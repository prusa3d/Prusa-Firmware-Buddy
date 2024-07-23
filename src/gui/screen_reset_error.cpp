// screen_reset_error.cpp

#include "screen_reset_error.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "version.h"
#include "support_utils.h"
#include <str_utils.hpp>

ScreenResetError::ScreenResetError(const Rect16 &fw_version_rect)
    : screen_t()
    , fw_version_txt(this, fw_version_rect, is_multiline::no)
    , sound_started(false) {

    ClrMenuTimeoutClose();
    ClrOnSerialClose();
    start_sound();

    fw_version_txt.set_font(Font::small);

    fw_version_txt.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::LeftTop() : Align_t::CenterTop());

    /// (fw version full string) [fw signed][appendix]
    const char *signed_str = signature_exist() ? "[S]" : "";
    const char *apendix_str = appendix_exist() ? "[A]" : "";
    StringBuilder(fw_version_str).append_printf("%s %s %s%s", project_version_full, PRINTER_MODEL, signed_str, apendix_str);
    fw_version_txt.SetText(string_view_utf8::MakeRAM(fw_version_str.data()));
}

void ScreenResetError::start_sound() {
    if (!sound_started) {
        /// avoid collision of sounds
        Sound_Stop();
        Sound_Play(eSOUND_TYPE::CriticalAlert);
        sound_started = true;
    }
}

void ScreenResetError::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
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
#if PRINTER_IS_PRUSA_MK4()
    static_assert(ERR_PRINTER_CODE == 13, "If ERR_PRINTER_CODE changes, please revisit this and check that everything's all right.");
    static constexpr EnumArray<ExtendedPrinterType, int32_t, extended_printer_type_count> error_code_offsets {
        { ExtendedPrinterType::mk4, (13 - ERR_PRINTER_CODE) * 1000 },
        { ExtendedPrinterType::mk4s, (21 - ERR_PRINTER_CODE) * 1000 },
        { ExtendedPrinterType::mk3_9, (26 - ERR_PRINTER_CODE) * 1000 },
    };
    error_code += error_code_offsets.get_fallback(config_store().extended_printer_type.get(), ExtendedPrinterType::mk4);
#endif
}
