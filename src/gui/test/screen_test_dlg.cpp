// screen_test_msgbox.cpp

#include "screen_test_dlg.hpp"
#include "config.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "window_dlg_strong_warning.hpp"

static const char *test_text = "Welcome to the Original Prusa MINI setup wizard. Would you like to continue?";
static const string_view_utf8 test_text_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_text));
static const char *test_header = "Header";
static const string_view_utf8 test_header_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_header));

screen_test_dlg_data_t::screen_test_dlg_data_t()
    : AddSuperWindow<screen_t>()
    , tst(this, Rect16(10, 32, 220, 22), is_multiline::no)
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes)
    , tst_usb_error(this, Rect16(10, 76, 220, 22), []() { window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::USBFlashDisk); })
    , tst_fan_error(this, Rect16(10, 98, 220, 22), []() { window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::HotendFan); })
    , tst_safety_timer(this, Rect16(10, 120, 220, 22), []() { window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::HeatersTimeout); }) {
    static const char tm[] = "TEST STRONG DIALOGS";
    tst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tm));

    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));

    static const char oc[] = "USB ERROR";
    tst_usb_error.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)oc));

    static const char er[] = "FAN ERROR";
    tst_fan_error.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)er));

    static const char qu[] = "SAFETY TIMER";
    tst_safety_timer.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)qu));
}
