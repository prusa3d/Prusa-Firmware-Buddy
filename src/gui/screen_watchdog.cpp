// screen_watchdog.c

#include "gui.h"
#include "config.h"
#include "screens.h"

typedef struct
{
    window_frame_t frame;
    window_text_t text0;
    window_text_t text1;
} screen_watchdog_data_t;

typedef struct _screen_watchdog_t {
    screen_t scr;
    screen_watchdog_data_t *pd;
} screen_watchdog_t;

void screen_watchdog_init(screen_watchdog_t *screen) {
    if (screen->pd == 0) {
        int16_t id;
        int16_t id0;
        screen_watchdog_data_t *pd = (screen_watchdog_data_t *)gui_malloc(sizeof(screen_watchdog_data_t));
        screen->pd = pd;

        id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));
        window_set_color_back(id0, COLOR_RED);

        id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 70, 220, 24), &(pd->text0));
        pd->text0.font = resource_font(IDR_FNT_BIG);
        window_set_text(id, "WATCHDOG RESET");
        window_set_alignment(id, ALIGN_CENTER);

        id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, 110, 240, 24), &(pd->text1));
        pd->text1.font = resource_font(IDR_FNT_NORMAL);
        window_set_text(id, "press to continue...");
        window_set_alignment(id, ALIGN_CENTER);
        window_enable(id);
        window_set_tag(id, 1);
    }
}

void screen_watchdog_done(screen_watchdog_t *screen) {
    if (screen->pd) {
        window_destroy(screen->pd->frame.win.id);
        gui_free(screen->pd);
        screen->pd = 0;
    }
}

void screen_watchdog_draw(screen_watchdog_t *screen) {
}

int screen_watchdog_event(screen_watchdog_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        screen_close();
    return 0;
}

screen_watchdog_t screen_watchdog = {
    {
        0,
        0,
        (screen_init_t *)screen_watchdog_init,
        (screen_done_t *)screen_watchdog_done,
        (screen_draw_t *)screen_watchdog_draw,
        (screen_event_t *)screen_watchdog_event,
        0, //data_size
        0, //pdata
    },
    0,
};

screen_t *const get_scr_watchdog() { return (screen_t *)&screen_watchdog; }
