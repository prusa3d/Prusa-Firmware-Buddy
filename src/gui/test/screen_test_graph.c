// screen_test_term.c

#include "gui.h"
#include "config.h"
#include "window_temp_graph.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include "screens.h"

extern void window_temp_scope_add(float temp_ext, float temp_bed);

typedef struct
{
    window_frame_t frame;
    window_text_t text;
    window_text_t button;
    int16_t id_frame;
    int16_t id_text;
    int16_t id_button;
    int16_t id_graph;
    window_temp_graph_t graph;
} screen_test_term_data_t;

typedef struct _screen_test_term_t {
    screen_t scr;
    screen_test_term_data_t *pd;
} screen_test_term_t;

extern osThreadId displayTaskHandle;

void screen_test_graph_init(screen_test_term_t *screen) {
    if (screen->pd == 0) {
        int16_t id;
        //font_t* font = resource_font(IDR_FNT_TERMINAL);
        screen_test_term_data_t *pd = (screen_test_term_data_t *)gui_malloc(sizeof(screen_test_term_data_t));
        screen->pd = pd;

        int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));
        pd->id_frame = id0;
        window_set_color_back(id0, COLOR_BLACK);

        id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 0, 220, 22), &(pd->text));
        pd->id_text = id;
        window_set_text(id, (const char *)"Test");

        id = window_create_ptr(WINDOW_CLS_TEMP_GRAPH, id0, rect_ui16(10, 28, 180, 180), &pd->graph);
        pd->id_graph = id;

        id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 220, 100, 22), &(pd->button));
        pd->id_button = id;
        window_set_text(id, (const char *)"Return");
        window_enable(id);
        window_set_tag(id, 1);
    }
}

void screen_test_graph_done(screen_test_term_t *screen) {
    if (screen->pd) {
        window_destroy(screen->pd->frame.win.id);
        gui_free(screen->pd);
        screen->pd = 0;
    }
}

void screen_test_graph_draw(screen_test_term_t *screen) {
}

uint8_t i = 0;

int screen_test_graph_event(screen_test_term_t *screen, window_t *window, uint8_t event, void *param) {
    //int winid = -1;
    //if (window) window->id;

    if (event == WINDOW_EVENT_LOOP) {
        if (i == 5) {
            screen->pd->graph.win.flg |= WINDOW_FLG_GRAPH_INVALID;
            //osSignalSet(displayTaskHandle, SIG_DISP_REDRAW);
            gui_invalidate();
            i = 0;
        }
        i++;
    } else if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case 1:
            //screen_open(get_scr_menu_service()->id);
            screen_close();
            break;
        }
    }
    return 0;
}

screen_test_term_t screen_test_graph = {
    {
        0,
        0,
        (screen_init_t *)screen_test_graph_init,
        (screen_done_t *)screen_test_graph_done,
        (screen_draw_t *)screen_test_graph_draw,
        (screen_event_t *)screen_test_graph_event,
        0, //data_size
        0, //pdata
    },
    0,
};

screen_t *const get_scr_test_graph() { return (screen_t *)(&screen_test_graph); }
