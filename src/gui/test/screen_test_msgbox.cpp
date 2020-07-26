// screen_test_msgbox.cpp

#include "screen_test_msgbox.hpp"
#include "config.h"
#include "../lang/i18n.h"
#include "ScreenHandler.hpp"

const char *test_text = N_("Welcome to the Original Prusa MINI setup wizard. Would you like to continue?");

screen_test_msgbox_data_t::screen_test_msgbox_data_t()
    : window_frame_t()
    , tst(this, rect_ui16(10, 32, 220, 22))
    , back(this, rect_ui16(10, 54, 220, 22), is_closed_on_click_t::yes)
    , tst_ok(this, rect_ui16(10, 76, 220, 22), is_closed_on_click_t::yes)
    , tst_okcancel(this, rect_ui16(10, 98, 220, 22), is_closed_on_click_t::yes)
    , tst_abortretryignore(this, rect_ui16(10, 120, 220, 22), is_closed_on_click_t::yes)
    , tst_yesnocancel(this, rect_ui16(10, 142, 220, 22), is_closed_on_click_t::yes)
    , tst_yesno(this, rect_ui16(10, 164, 220, 22), is_closed_on_click_t::yes)
    , tst_retrycancel(this, rect_ui16(10, 186, 220, 22), is_closed_on_click_t::yes)
    , tst_ico_custom(this, rect_ui16(10, 208, 220, 22), is_closed_on_click_t::yes)
    , tst_ico_error(this, rect_ui16(10, 230, 220, 22), is_closed_on_click_t::yes)
    , tst_ico_question(this, rect_ui16(10, 252, 220, 22), is_closed_on_click_t::yes)
    , tst_ico_warning(this, rect_ui16(10, 276, 220, 22), is_closed_on_click_t::yes)
    , tst_ico_info(this, rect_ui16(10, 298, 220, 22), is_closed_on_click_t::yes) {

    static const char tm[] = "TEST MSGBOX";
    tst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tm));

    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));

    static const char ok[] = "OK";
    tst_ok.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ok));

    static const char oc[] = "OK-CANCEL";
    tst_okcancel.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)oc));

    static const char ari[] = "ABORT-RETRY-IGNORE";
    tst_abortretryignore.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ari));

    static const char ync[] = "YES-NO-CANCEL";
    tst_yesnocancel.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ync));

    static const char yn[] = "YES-NO";
    tst_yesno.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)yn));

    static const char rc[] = "RETRY-CANCEL";
    tst_retrycancel.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)rc));

    static const char cu[] = "CUSTOM";
    tst_ico_custom.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)cu));

    static const char er[] = "ERROR";
    tst_ico_error.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)er));

    static const char qu[] = "QUESTION";
    tst_ico_question.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)qu));

    static const char wa[] = "WARNING";
    tst_ico_warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wa));
}
/*
void screen_test_msgbox_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case MSGBOX_BTN_MAX + 2:
            Screens::Access()->Close();
            return;
        case MSGBOX_BTN_OK + 1:
        case MSGBOX_BTN_OKCANCEL + 1:
        case MSGBOX_BTN_ABORTRETRYIGNORE + 1:
        case MSGBOX_BTN_YESNOCANCEL + 1:
        case MSGBOX_BTN_YESNO + 1:
        case MSGBOX_BTN_RETRYCANCEL + 1: {
            uint16_t btn = ((int)param - 1) & MSGBOX_MSK_BTN;
            gui_msgbox(_(test_text), btn | MSGBOX_ICO_INFO);
        } break;
        case MSGBOX_BTN_MAX + 3:
        case MSGBOX_BTN_MAX + 4:
        case MSGBOX_BTN_MAX + 5:
        case MSGBOX_BTN_MAX + 6: {
            uint16_t ico = (((int)param - (MSGBOX_BTN_MAX + 3)) << MSGBOX_SHI_ICO) & MSGBOX_MSK_ICO;
            gui_msgbox(_(test_text), MSGBOX_BTN_OK | ico);
        } break;
        }
    else {
        window_frame_t::windowEvent(sender, event, param);
    }
}
*/
