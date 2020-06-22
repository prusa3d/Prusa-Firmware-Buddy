#include "gui.h"
#include "config.h"
#include <stdlib.h>
#include "support_utils.h"

#include "display.h"
#include "errors.h"
#include "screens.h"

struct screen_qr_error_data_t {
    window_frame_t root;
    window_text_t errText;
    window_text_t errDescription;
    window_text_t info;
    window_qr_t qr;
    char qr_text[MAX_LEN_4QR + 1];
    bool first_run_flag;
};

#define pd ((screen_qr_error_data_t *)screen->pdata)

void screen_menu_qr_error_init(screen_t *screen) {
    int16_t id, root;

    root = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->root));
    window_set_color_back(root, COLOR_RED_ALERT);

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 0, 224, 25), &(pd->errText));
    window_set_color_back(id, COLOR_RED_ALERT);
    pd->errText.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, get_actual_error()->err_title);

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 30, 224, 95), &(pd->errDescription));
    window_set_color_back(id, COLOR_RED_ALERT);
    window_set_text(id, get_actual_error()->err_text);

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 275, 224, 20), &(pd->info));
    window_set_color_back(id, COLOR_RED_ALERT);
    window_set_alignment(id, ALIGN_CENTER);
    window_set_text(id, "help.prusa3d.com");

    id = window_create_ptr(WINDOW_CLS_QR, root, rect_ui16(59, 140, 224, 95), &(pd->qr));
    pd->qr.px_per_module = 2;
    create_path_info_4error(pd->qr_text, MAX_LEN_4QR + 1, 1);
    pd->qr.text = pd->qr_text;

    pd->first_run_flag = true;
}

void screen_menu_qr_error_draw(screen_t * /*screen*/) {
    display::FillRect(rect_ui16(8, 25, 224, 2), COLOR_WHITE);
}

void screen_menu_qr_error_done(screen_t *screen) {
    window_destroy(pd->root.win.id);
}

int screen_menu_qr_error_event(screen_t *screen, window_t * /*window*/, uint8_t event, void * /*param*/) {
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

screen_t screen_qr_error = {
    0,
    0,
    screen_menu_qr_error_init,
    screen_menu_qr_error_done,
    screen_menu_qr_error_draw,
    screen_menu_qr_error_event,
    sizeof(screen_qr_error_data_t), //data_size
    nullptr,                        //pdata
};

screen_t *const get_scr_qr_error() { return &screen_qr_error; }
