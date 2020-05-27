#include "gui.h"
#include "config.h"
#include "screen_menu.h"
#include <stdlib.h>
#include "support_utils.h"
#include "qrcodegen.h"
#include "qrcodegen_utils.h"

#include "../../gui/wizard/selftest.h"
#include "stm32f4xx_hal.h"

#pragma pack(push, 1)
typedef struct
{
    window_frame_t root;
    window_text_t warning;
    window_text_t button;
    window_qr_t qr;
    char qr_text[grcodegen_getDataSize(9, qrcodegen_Ecc_MEDIUM, qrcodegen_Mode_ALPHANUMERIC) + 1];
} screen_qr_info_data_t;
#pragma pack(pop)

#define pd ((screen_qr_info_data_t *)screen->pdata)

/// screen-init call-back
void screen_menu_qr_info_init(screen_t *screen) {
    int16_t id, root;

    root = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->root));

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 25, 224, 95), &(pd->warning));
    pd->warning.font = resource_font(IDR_FNT_TERMINAL);
    window_set_alignment(id, ALIGN_HCENTER);
    if (last_selftest_time == 0)
        window_set_text(id, "selfTest-data not\n    available");
    else if ((HAL_GetTick() / 1000 - last_selftest_time) > LAST_SELFTEST_TIMEOUT)
        window_set_text(id, "selfTest-data expired");
    else
        window_set_text(id, "selfTest-data relevant");

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(8, 280, 224, 30), &(pd->button));
    pd->button.font = resource_font(IDR_FNT_BIG);
    window_set_color_back(id, COLOR_ORANGE);
    window_set_color_text(id, COLOR_BLACK);
    window_set_alignment(id, ALIGN_HCENTER);
    window_set_text(id, "RETURN");

    id = window_create_ptr(WINDOW_CLS_QR, root, rect_ui16(28, 85, 224, 95), &(pd->qr));
    pd->qr.ecc_level = qrcodegen_Ecc_MEDIUM;
    create_path_info_4service(pd->qr_text);
    pd->qr.text = pd->qr_text;
}

/// screen-draw call-back
void screen_menu_qr_info_draw(screen_t *screen) {
}

/// screen-done call-back
void screen_menu_qr_info_done(screen_t *screen) {
    window_destroy(pd->root.win.id);
}

/// screen-event call-back
int screen_menu_qr_info_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if ((event == WINDOW_EVENT_CLICK) || (event == WINDOW_EVENT_BTN_DN)) {
        screen_close();
        return (1);
    }
    return (0);
}

/// screen definition
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

const screen_t *pscreen_qr_info = &screen_qr_info;
