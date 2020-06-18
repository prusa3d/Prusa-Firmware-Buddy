// screen_test_term.c

#include "gui.h"
#include "config.h"
#include "window_progress.h"
#include "screens.h"

#include "stm32f4xx_hal.h"

typedef struct
{
    window_frame_t frame;
    window_text_t text;
    window_term_t term;
    int16_t id_frame;
    int16_t id_text;
    int16_t id_term;
    term_t terminal;
    uint8_t term_buff[TERM_BUFF_SIZE(20, 16)]; //chars and attrs (640 bytes) + change bitmask (40 bytes)
} screen_test_term_data_t;

#define pd ((screen_test_term_data_t *)screen->pdata)

void screen_test_term_init(screen_t *screen) {
    int16_t id;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));
    pd->id_frame = id0;
    window_set_color_back(id0, COLOR_GRAY);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 0, 220, 22), &(pd->text));
    pd->id_text = id;
    window_set_text(id, (const char *)"Test");

    id = window_create_ptr(WINDOW_CLS_TERM, id0, rect_ui16(10, 28, 11 * 20, 18 * 16), &(pd->term));
    pd->id_term = id;
    term_init(&(pd->terminal), 20, 16, pd->term_buff);
    pd->term.term = &(pd->terminal);
}

void screen_test_term_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
}

void screen_test_term_draw(screen_t *screen) {
}

int screen_test_term_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    int winid = -1;
    if (event == WINDOW_EVENT_BTN_DN) {
        screen_close();
        return 1;
    }
    if (event != WINDOW_EVENT_LOOP) {
        term_printf(pd->term.term, "%010d w:%d e:%d\n", HAL_GetTick(), winid, (int)event);
        //	else
        //		if (pd->term.term->flg & TERM_FLG_CHANGED)
        window_invalidate(pd->term.win.id);
    }
    return 0;
}

screen_t screen_test_term = {
    0,
    0,
    screen_test_term_init,
    screen_test_term_done,
    screen_test_term_draw,
    screen_test_term_event,
    sizeof(screen_test_term_data_t), //data_size
    0,                               //pdata
};

screen_t *const get_scr_test_term() { return &screen_test_term; }
