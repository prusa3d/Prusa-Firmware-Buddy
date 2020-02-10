// screen_test.c

#include "gui.h"
#include "config.h"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "bsod.h"

extern screen_t *pscreen_test_gui;
extern screen_t *pscreen_test_term;
extern screen_t *pscreen_test_msgbox;
extern screen_t *pscreen_test_graph;
extern screen_t *pscreen_test_temperature;
extern screen_t *pscreen_test_disp_mem;

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_frame_t frame;
    window_text_t tst;
    window_text_t back;
    window_text_t tst_gui;
    window_text_t tst_term;
    window_text_t tst_msgbox;
    window_text_t tst_graph;
    window_text_t tst_temperature;
    window_text_t tst_heat_err;
    window_text_t tst_disp_memory;
    window_text_t tst_stack_overflow;
    int8_t id_tim;
    int8_t id_tim1;
} screen_test_data_t;

#pragma pack(pop)

#define pd ((screen_test_data_t *)screen->pdata)

typedef enum {
	STI_back = 1,
	STI_tst_gui,
	STI_tst_term,
	STI_tst_msgbox,
	STI_tst_graph,
	STI_tst_temperature,
	STI_tst_heat_err,
	STI_tst_disp_memory,
	STI_tst_stack_overflow

}STI_tag_t;

void screen_test_init(screen_t *screen) {
    int16_t id;

    int16_t y = 32;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst));
    window_set_text(id, (const char *)"TEST");
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->back));
    window_set_text(id, (const char *)"back");
    window_enable(id);
    window_set_tag(id, STI_back);
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_gui));
    window_set_text(id, (const char *)"test GUI");
    window_enable(id);
    window_set_tag(id, STI_tst_gui);
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_term));
    window_set_text(id, (const char *)"test TERM");
    window_enable(id);
    window_set_tag(id, STI_tst_term);
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_msgbox));
    window_set_text(id, (const char *)"test MSGBOX");
    window_enable(id);
    window_set_tag(id, STI_tst_msgbox);
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_graph));
    window_set_text(id, (const char *)"temp graph");
    window_enable(id);
    window_set_tag(id, STI_tst_graph);
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_temperature));
    window_set_text(id, (const char *)"temp - pwm");
    window_enable(id);
    window_set_tag(id, STI_tst_temperature);
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_heat_err));
    window_set_text(id, (const char *)"HEAT ERROR");
    window_enable(id);
    window_set_tag(id, STI_tst_heat_err);
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_disp_memory));
    window_set_text(id, (const char *)"Disp. R/W");
    window_enable(id);
    window_set_tag(id, STI_tst_disp_memory);
    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_stack_overflow));
    window_set_text(id, (const char *)"Stack overflow");
    window_enable(id);
    window_set_tag(id, STI_tst_stack_overflow);

    pd->id_tim = gui_timer_create_oneshot(2000, id0);
    pd->id_tim1 = gui_timer_create_periodical(4000, id0);
}

void screen_test_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
}

void screen_test_draw(screen_t *screen) {
}


static volatile int _recursive = 1;
static volatile void recursive(uint64_t i){
	uint64_t x = i + (uint64_t)_recursive;
	osDelay(1);
	if(_recursive)recursive(x);
}

int screen_test_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case STI_back:
            screen_close();
            return 1;
        case STI_tst_gui:
            screen_open(pscreen_test_gui->id);
            return 1;
        case STI_tst_term:
            screen_open(pscreen_test_term->id);
            return 1;
        case STI_tst_msgbox:
            screen_open(pscreen_test_msgbox->id);
            return 1;
        case STI_tst_graph:
            screen_open(pscreen_test_graph->id);
            return 1;
        case STI_tst_temperature:
            screen_open(pscreen_test_temperature->id);
            return 1;
        case STI_tst_heat_err:
            temp_error("TEST BED ERROR", "Bed",1.0,2.0,3.0,4.0);
            return 1;
        case STI_tst_disp_memory:
            screen_open(pscreen_test_disp_mem->id);
            return 1;
        case STI_tst_stack_overflow:
        	recursive(0);
        	return 1;
        }
    else if (event == WINDOW_EVENT_TIMER) {
        if ((int)param == pd->id_tim)
            _dbg("tim0 %lu", HAL_GetTick());
        else if ((int)param == pd->id_tim1)
            _dbg("tim1 %lu", HAL_GetTick());
        return 1;
    }
    return 0;
}

screen_t screen_test = {
    0,
    0,
    screen_test_init,
    screen_test_done,
    screen_test_draw,
    screen_test_event,
    sizeof(screen_test_data_t), //data_size
    0, //pdata
};

const screen_t *pscreen_test = &screen_test;
