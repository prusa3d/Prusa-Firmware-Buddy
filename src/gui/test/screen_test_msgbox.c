// screen_test_msgbox.c

#include "gui.h"
#include "config.h"
#include "screens.h"
#include "../lang/i18n.h"

typedef struct
{
    window_frame_t frame;
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
} screen_test_msgbox_data_t;

#define pd ((screen_test_msgbox_data_t *)screen->pdata)

const char *test_text = N_("Welcome to the Original Prusa MINI setup wizard. Would you like to continue?");

void screen_test_msgbox_init(screen_t *screen) {
    int16_t id;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 32, 220, 22), &(pd->tst));
    window_set_text(id, (const char *)"TEST MSGBOX");

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 54, 220, 22), &(pd->back));
    window_set_text(id, (const char *)"back");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_MAX + 2);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 76, 220, 22), &(pd->tst_ok));
    window_set_text(id, (const char *)"OK");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_OK + 1);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 98, 220, 22), &(pd->tst_okcancel));
    window_set_text(id, (const char *)"OK-CANCEL");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_OKCANCEL + 1);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 120, 220, 22), &(pd->tst_abortretryignore));
    window_set_text(id, (const char *)"ABORT-RETRY-IGNORE");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_ABORTRETRYIGNORE + 1);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 142, 220, 22), &(pd->tst_yesnocancel));
    window_set_text(id, (const char *)"YES-NO-CANCEL");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_YESNOCANCEL + 1);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 164, 220, 22), &(pd->tst_yesno));
    window_set_text(id, (const char *)"YES-NO");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_YESNO + 1);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 186, 220, 22), &(pd->tst_retrycancel));
    window_set_text(id, (const char *)"RETRY-CANCEL");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_RETRYCANCEL + 1);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(20, 208, 90, 22), &(pd->tst_ico_custom));
    window_set_text(id, (const char *)"CUSTOM");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_MAX + 3);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(20, 230, 90, 22), &(pd->tst_ico_error));
    window_set_text(id, (const char *)"ERROR");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_MAX + 4);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(110, 208, 90, 22), &(pd->tst_ico_question));
    window_set_text(id, (const char *)"QUESTION");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_MAX + 5);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(110, 230, 90, 22), &(pd->tst_ico_warning));
    window_set_text(id, (const char *)"WARNING");
    window_enable(id);
    window_set_tag(id, MSGBOX_BTN_MAX + 6);
}

void screen_test_msgbox_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
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
    /*	else if (event == WINDOW_EVENT_LOOP)
	{
		float temp = window_get_value(screen->pd->spin0.window.win.id);
		int val = sim_heater_temp2val(temp);
		window_set_value(screen->pd->numb0.win.id, val);
	}*/
    return 0;
}

screen_t screen_test_msgbox = {
    0,
    0,
    screen_test_msgbox_init,
    screen_test_msgbox_done,
    screen_test_msgbox_draw,
    screen_test_msgbox_event,
    sizeof(screen_test_msgbox_data_t), //data_size
    0,                                 //pdata
};

screen_t *const get_scr_test_msgbox() { return &screen_test_msgbox; }
