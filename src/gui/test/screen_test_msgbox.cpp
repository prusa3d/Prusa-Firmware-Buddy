// screen_test_msgbox.cpp

#include "gui.hpp"
#include "config.h"
#include "screens.h"
#include "../lang/i18n.h"

struct screen_test_msgbox_data_t : public window_frame_t {
    window_text_t tst;
    window_text_t back;
    window_text_t tst_ok;
    window_text_t tst_okcancel;
    window_text_t tst_abortretryignore;
    window_text_t tst_yesnocancel;
    window_text_t tst_yesno;
    window_text_t tst_retrycancel;
    window_text_t tst_ico_custom;
    window_text_t tst_ico_error;
    window_text_t tst_ico_question;
    window_text_t tst_ico_warning;
    window_text_t tst_ico_info;
};

#define pd ((screen_test_msgbox_data_t *)screen->pdata)

const char *test_text = N_("Welcome to the Original Prusa MINI setup wizard. Would you like to continue?");

void screen_test_msgbox_init(screen_t *screen) {
    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), pd);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 32, 220, 22), &(pd->tst));
    static const char tm[] = "TEST MSGBOX";
    pd->tst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tm));

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 54, 220, 22), &(pd->back));
    static const char bck[] = "back";
    pd->back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));
    pd->back.Enable();
    pd->back.SetTag(MSGBOX_BTN_MAX + 2);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 76, 220, 22), &(pd->tst_ok));
    static const char ok[] = "OK";
    pd->tst_ok.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ok));
    pd->tst_ok.Enable();
    pd->tst_ok.SetTag(MSGBOX_BTN_OK + 1);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 98, 220, 22), &(pd->tst_okcancel));
    static const char oc[] = "OK-CANCEL";
    pd->tst_okcancel.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)oc));
    pd->tst_okcancel.Enable();
    pd->tst_okcancel.SetTag(MSGBOX_BTN_OKCANCEL + 1);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 120, 220, 22), &(pd->tst_abortretryignore));
    static const char ari[] = "ABORT-RETRY-IGNORE";
    pd->tst_abortretryignore.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ari));
    pd->tst_abortretryignore.Enable();
    pd->tst_abortretryignore.SetTag(MSGBOX_BTN_ABORTRETRYIGNORE + 1);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 142, 220, 22), &(pd->tst_yesnocancel));
    static const char ync[] = "YES-NO-CANCEL";
    pd->tst_yesnocancel.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ync));
    pd->tst_yesnocancel.Enable();
    pd->tst_yesnocancel.SetTag(MSGBOX_BTN_YESNOCANCEL + 1);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 164, 220, 22), &(pd->tst_yesno));
    static const char yn[] = "YES-NO";
    pd->tst_yesno.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)yn));
    pd->tst_yesno.Enable();
    pd->tst_yesno.SetTag(MSGBOX_BTN_YESNO + 1);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 186, 220, 22), &(pd->tst_retrycancel));
    static const char rc[] = "RETRY-CANCEL";
    pd->tst_retrycancel.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)rc));
    pd->tst_retrycancel.Enable();
    pd->tst_retrycancel.SetTag(MSGBOX_BTN_RETRYCANCEL + 1);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(20, 208, 90, 22), &(pd->tst_ico_custom));
    static const char cu[] = "CUSTOM";
    pd->tst_ico_custom.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)cu));
    pd->tst_ico_custom.Enable();
    pd->tst_ico_custom.SetTag(MSGBOX_BTN_MAX + 3);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(20, 230, 90, 22), &(pd->tst_ico_error));
    static const char er[] = "ERROR";
    pd->tst_ico_error.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)er));
    pd->tst_ico_error.Enable();
    pd->tst_ico_error.SetTag(MSGBOX_BTN_MAX + 4);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(110, 208, 90, 22), &(pd->tst_ico_question));
    static const char qu[] = "QUESTION";
    pd->tst_ico_question.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)qu));
    pd->tst_ico_question.Enable();
    pd->tst_ico_question.SetTag(MSGBOX_BTN_MAX + 5);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(110, 230, 90, 22), &(pd->tst_ico_warning));
    static const char wa[] = "WARNING";
    pd->tst_ico_warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wa));
    pd->tst_ico_warning.Enable();
    pd->tst_ico_warning.SetTag(MSGBOX_BTN_MAX + 6);
}

void screen_test_msgbox_done(screen_t *screen) {
    window_destroy(pd->id);
}

void screen_test_msgbox_draw(screen_t *screen) {
}

int screen_test_msgbox_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case MSGBOX_BTN_MAX + 2:
            screen_close();
            return 1;
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
    return 0;
}
