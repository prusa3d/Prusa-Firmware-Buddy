// screen_test_msgbox.cpp

#include "screen_test_msgbox.hpp"
#include "config.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "window_msgbox.hpp"

static const char *test_text = "Welcome to the Original Prusa MINI setup wizard. Would you like to continue?";
static const string_view_utf8 test_text_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_text));
static const char *test_header = "Header";
static const string_view_utf8 test_header_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_header));

screen_test_msgbox_data_t::screen_test_msgbox_data_t()
    : AddSuperWindow<screen_t>()
    , tst(this, Rect16(10, 32, 220, 22), is_multiline::no)
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes)
    , tst_ok(this, Rect16(10, 76, 220, 22), []() { MsgBox(test_text_view, Responses_Ok); })
    , tst_okcancel(this, Rect16(10, 98, 220, 22), []() { MsgBox(test_text_view, Responses_OkCancel); })
    , tst_ico_error(this, Rect16(10, 120, 220, 22), []() { MsgBoxError(test_text_view, Responses_AbortRetryIgnore); })
    , tst_ico_question(this, Rect16(10, 142, 220, 22), []() { MsgBoxQuestion(test_text_view, Responses_YesNoCancel); })
    , tst_ico_warning(this, Rect16(10, 164, 220, 22), []() { MsgBoxWarning(test_text_view, Responses_YesNo); })
    , tst_ico_info(this, Rect16(10, 186, 220, 22), []() { MsgBoxQuestion(test_text_view, Responses_RetryCancel); })
    , tst_icon(this, Rect16(10, 208, 220, 22), []() { MsgBoxPepa(test_text_view); }) {
    static const char tm[] = "TEST MSGBOX";
    tst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tm));

    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));

    static const char ok[] = "OK";
    tst_ok.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ok));

    static const char oc[] = "OK-CANCEL";
    tst_okcancel.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)oc));

    static const char er[] = "ERROR";
    tst_ico_error.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)er));

    static const char qu[] = "QUESTION";
    tst_ico_question.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)qu));

    static const char wa[] = "WARNING";
    tst_ico_warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wa));

    static const char in[] = "INFO";
    tst_ico_info.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)in));

    static const char ic[] = "ICON";
    tst_icon.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ic));
}
