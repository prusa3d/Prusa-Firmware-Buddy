// screen_marlin.c

#include "gui.h"
#include "config.h"
#include "cmsis_os.h"
#include "hwio.h"
#include "screens.h"

extern osThreadId displayTaskHandle;

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_frame_t frame;
    window_icon_t logo_prusa_mini;
    window_lcdsim_t lcdsim;
} screen_marlin_data_t;

typedef struct _screen_marlin_t {
    screen_t scr;
    int lcdsim_initialized;
} screen_marlin_t;

#pragma pack(pop)

extern void pngview(void);

screen_marlin_t screen_marlin;
screen_t *const get_scr_marlin() { return (screen_t *)&screen_marlin; }

#define _psd ((screen_marlin_data_t *)screen_marlin.scr.pdata)

void screen_marlin_init(screen_marlin_t *screen) {
    int16_t id;
    int16_t id0;

    id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(_psd->frame));
    //window_set_color_back(id0, COLOR_GRAY);

    id = window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(0, 20, 240, 64), &(_psd->logo_prusa_mini));
    window_set_tag(id, 1);
    //window_enable(id);

    pngview();
}

void screen_marlin_done(screen_marlin_t *screen) {
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
