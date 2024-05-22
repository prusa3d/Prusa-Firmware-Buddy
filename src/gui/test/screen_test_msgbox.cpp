/**
 * @file screen_test_msgbox.cpp
 */

#include "screen_test_msgbox.hpp"
#include "i18n.h"
#include "window_msgbox.hpp"
#include "client_response.hpp"

static const char *test_text = "Welcome to the Original Prusa MINI setup wizard. Would you like to continue?";
static const string_view_utf8 test_text_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_text));
static const char *test_header = "Header";
static const string_view_utf8 test_header_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_header));
static const char *test_title = "The title of the all titles";
static const string_view_utf8 test_title_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_title));
static const char *test_fin = "All tests finished successfully!";
static const string_view_utf8 test_fin_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_fin));
static const char *test_title2 = "QUEUE FULL";
static const string_view_utf8 test_title2_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(test_title2));

ScreenTestMSGBox::ScreenTestMSGBox()
    : AddSuperWindow<screen_t>()
    , header(this, string_view_utf8::MakeCPUFLASH((uint8_t *)"TEST MSGBOX"))
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes)
    , tst_ok(this, Rect16(10, 76, 220, 22), []() { MsgBox(test_text_view, Responses_Ok); })
    , tst_ico_error(this, Rect16(10, 120, 220, 22), []() { MsgBoxError(test_text_view, Responses_AbortRetryIgnore); })
    , tst_ico_question(this, Rect16(10, 142, 220, 22), []() { MsgBoxQuestion(test_text_view, Responses_YesNoCancel); })
    , tst_ico_warning(this, Rect16(10, 164, 220, 22), []() { MsgBoxWarning(test_text_view, Responses_YesNo); })
    , tst_ico_info(this, Rect16(10, 186, 220, 22), []() { MsgBoxInfo(test_text_view, Responses_RetryCancel); })
    , tst_icon(this, Rect16(10, 208, 220, 22), []() { MsgBoxPepa(test_fin_view, Responses_Ok); }) {
    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));

    static const char ok[] = "OK";
    tst_ok.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ok));

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
