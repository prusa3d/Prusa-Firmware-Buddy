#include "gui.h"
#include "config.h"
#include "screen_menu.h"
#include <stdlib.h>
#include "support_utils.h"
#include "str_utils.h"
#include "qrcodegen.h"
#include "qrcodegen_utils.h"

#include "display.h"
#include "errors.h"
#include "lang.h"

#define MAX_LINE_WIDTH       19
#define MAX_MULTILINE_LENGTH 255

#pragma pack(push, 1)
typedef struct
{
    window_frame_t root;
    window_text_t errText;
    window_text_t errDescription;
    window_text_t info;
    window_qr_t qr;
    char ml_text[MAX_MULTILINE_LENGTH];
    char qr_text[grcodegen_getDataSize(9, qrcodegen_Ecc_HIGH, qrcodegen_Mode_ALPHANUMERIC) + 1];
    bool first_run_flag;
} screen_qr_error_data_t;
#pragma pack(pop)

#define pd ((screen_qr_error_data_t *)screen->pdata)

/// screen-init call-back
void screen_menu_qr_error_init(screen_t *screen) {
    int16_t id, root;
    const err_t *perr;

    perr = get_actual_error();
    root = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->root));
    window_set_color_back(root, COLOR_RED_ALERT);

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 0, 224, 25), &(pd->errText));
    window_set_color_back(id, COLOR_RED_ALERT);
    pd->errText.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, perr->err_title);

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 30, 224, 95), &(pd->errDescription));
    window_set_color_back(id, COLOR_RED_ALERT);
    strcpy(pd->ml_text, perr->err_text);
    str2multiline(pd->ml_text, MAX_LINE_WIDTH);
    window_set_text(id, pd->ml_text);

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 275, 224, 20), &(pd->info));
    window_set_color_back(id, COLOR_RED_ALERT);
    window_set_alignment(id, ALIGN_CENTER);
    window_set_text(id, get_actual_lang()->help_text);

    id = window_create_ptr(WINDOW_CLS_QR, root, rect_ui16(59, 140, 224, 95), &(pd->qr));
    pd->qr.px_per_module = 2;
    create_path_info_4error(pd->qr_text, perr->err_ext_code);
    pd->qr.text = pd->qr_text;

    pd->first_run_flag = true;
}

/// screen-draw call-back
void screen_menu_qr_error_draw(screen_t *screen) {
    display->fill_rect(rect_ui16(8, 25, 224, 2), COLOR_WHITE);
}

/// screen-done call-back
void screen_menu_qr_error_done(screen_t *screen) {
    window_destroy(pd->root.win.id);
}

/// screen-event call-back
int screen_menu_qr_error_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if ((event == WINDOW_EVENT_CLICK) || (event == WINDOW_EVENT_BTN_DN)) {
        screen_close();
        return (1);
    }
    if (!pd->first_run_flag)
        return (0);
    pd->first_run_flag = false;
    screen_menu_qr_error_draw(screen);
    return (0);
}

/// screen definition
screen_t screen_qr_error = {
    0,
    0,
    screen_menu_qr_error_init,
    screen_menu_qr_error_done,
    screen_menu_qr_error_draw,
    screen_menu_qr_error_event,
    sizeof(screen_qr_error_data_t), //data_size
    0,                              //pdata
};

const screen_t *pscreen_qr_error = &screen_qr_error;
