// screen_marlin.c

#include "gui.h"
#include "config.h"
#include "window_lcdsim.h"
#include "window_logo.h"
#include "cmsis_os.h"
#include "hwio.h"
#include "lcdsim.h"

extern osThreadId displayTaskHandle;

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_frame_t frame;
    window_logo_t logo_prusa_mini;
    window_lcdsim_t lcdsim;
} screen_marlin_data_t;

typedef struct _screen_marlin_t {
    screen_t scr;
    int lcdsim_initialized;
} screen_marlin_t;

#pragma pack(pop)

extern void pngview(void);

screen_marlin_t screen_marlin;
const screen_t *pscreen_marlin = (screen_t *)&screen_marlin;

#define _psd ((screen_marlin_data_t *)screen_marlin.scr.pdata)

void screen_marlin_init(screen_marlin_t *screen) {
    int16_t id;
    int16_t id0;

    id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(_psd->frame));
    //window_set_color_back(id0, COLOR_GRAY);

    id = window_create_ptr(WINDOW_CLS_LOGO, id0, rect_ui16(0, 20, 240, 64), &(_psd->logo_prusa_mini));
    window_set_tag(id, 1);
    //window_enable(id);

#ifdef LCDSIM
    //#if 0
    if (!screen->lcdsim_initialized) {
        lcdsim_init();
        screen->lcdsim_initialized = 1;
    } else
        lcdsim_invalidate();
    if (WINDOW_CLS_LCDSIM == 0)
        WINDOW_CLS_LCDSIM = window_register_class((window_class_t *)&window_class_lcdsim);
    id = window_create_ptr(WINDOW_CLS_LCDSIM, id0, rect_ui16(0, 96, 240, 72), &(_psd->lcdsim));
    hwio_jogwheel_enable();
#else
    pngview();
#endif //LCDSIM
}

void screen_marlin_done(screen_marlin_t *screen) {
#ifdef LCDSIM
    hwio_jogwheel_disable();
#endif //LCDSIM
    window_destroy(_psd->frame.win.id);
}

void screen_marlin_draw(screen_marlin_t *screen) {
}

int screen_marlin_event(screen_marlin_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_LOOP)
        window_invalidate(_psd->lcdsim.win.id);
    return 0;
}

screen_marlin_t screen_marlin = {
    {
        0,
        0,
        (screen_init_t *)screen_marlin_init,
        (screen_done_t *)screen_marlin_done,
        (screen_draw_t *)screen_marlin_draw,
        (screen_event_t *)screen_marlin_event,
        sizeof(screen_marlin_data_t), //data_size
        0,                            //pdata
    },
    0, //lcdsim_initialized
};
