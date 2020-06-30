#include "gui.hpp"
#include "config.h"
#include <stdlib.h>
#include "support_utils.h"
#include "screens.h"

#include "../../gui/wizard/selftest.h"
#include "stm32f4xx_hal.h"

struct screen_qr_info_data_t {
    window_frame_t root;
    window_text_t warning;
    window_text_t button;
    window_qr_t qr;
    char qr_text[MAX_LEN_4QR + 1];
};

#define pd ((screen_qr_info_data_t *)screen->pdata)

void screen_menu_qr_info_init(screen_t *screen) {
    int16_t root;

    root = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->root));

    window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 25, 224, 95), &(pd->warning));
    pd->warning.font = resource_font(IDR_FNT_TERMINAL);
    pd->warning.SetAlignment(ALIGN_HCENTER);
    static const char slftNA[] = "selfTest-data not\n    available";
    static const char slftEx[] = "selfTest-data expired";
    static const char slftRe[] = "selfTest-data relevant";
    if (last_selftest_time == 0)
        pd->warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)slftNA));
    else if ((HAL_GetTick() / 1000 - last_selftest_time) > LAST_SELFTEST_TIMEOUT)
        pd->warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)slftEx));
    else
        pd->warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)slftRe));

    window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 280, 224, 30), &(pd->button));
    pd->button.font = resource_font(IDR_FNT_BIG);
    pd->button.SetBackColor(COLOR_WHITE);
    pd->button.SetTextColor(COLOR_BLACK);
    pd->button.SetAlignment(ALIGN_HCENTER);
    static const char rtn[] = "RETURN";
    pd->button.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)rtn));

    window_create_ptr(WINDOW_CLS_QR, root, rect_ui16(28, 85, 224, 95), &(pd->qr));
    pd->qr.ecc_level = qrcodegen_Ecc_MEDIUM;
    create_path_info_4service(pd->qr_text, MAX_LEN_4QR + 1);
    pd->qr.text = pd->qr_text;
}

void screen_menu_qr_info_draw(screen_t *screen) {
}

void screen_menu_qr_info_done(screen_t *screen) {
    window_destroy(pd->root.id);
}

int screen_menu_qr_info_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if ((event == WINDOW_EVENT_CLICK) || (event == WINDOW_EVENT_BTN_DN)) {
        screen_close();
        return (1);
    }
    return (0);
}

screen_t screen_qr_info = {
    0,
    0,
    screen_menu_qr_info_init,
    screen_menu_qr_info_done,
    screen_menu_qr_info_draw,
    screen_menu_qr_info_event,
    sizeof(screen_qr_info_data_t), //data_size
    0,                             //pdata
};

screen_t *const get_scr_qr_info() { return &screen_qr_info; }
