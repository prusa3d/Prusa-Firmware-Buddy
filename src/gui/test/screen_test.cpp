// screen_test.cpp

#include "gui.hpp"
#include "config.h"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "bsod.h"
#include "screens.h"

struct screen_test_data_t : public window_frame_t {
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
};

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
} STI_tag_t;

void screen_test_init(screen_t *screen) {
    int16_t y = 32;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), pd);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst));
    static const char tst[] = "TEST";
    pd->tst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tst));
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->back));
    static const char bck[] = "back";
    pd->back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));
    pd->back.Enable();
    pd->back.SetTag(STI_back);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_gui));
    static const char tstg[] = "test GUI";
    pd->tst_gui.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstg));
    pd->tst_gui.Enable();
    pd->tst_gui.SetTag(STI_tst_gui);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_term));
    static const char tstt[] = "test TERM";
    pd->tst_term.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstt));
    pd->tst_term.Enable();
    pd->tst_term.SetTag(STI_tst_term);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_msgbox));
    static const char tstm[] = "test MSGBOX";
    pd->tst_msgbox.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstm));
    pd->tst_msgbox.Enable();
    pd->tst_msgbox.SetTag(STI_tst_msgbox);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_graph));
    static const char tmpg[] = "temp graph";
    pd->tst_graph.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tmpg));
    pd->tst_graph.Enable();
    pd->tst_graph.SetTag(STI_tst_graph);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_temperature));
    static const char tmpp[] = "temp - pwm";
    pd->tst_temperature.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tmpp));
    pd->tst_temperature.Enable();
    pd->tst_temperature.SetTag(STI_tst_temperature);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_heat_err));
    static const char he[] = "HEAT ERROR";
    pd->tst_heat_err.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)he));
    pd->tst_heat_err.Enable();
    pd->tst_heat_err.SetTag(STI_tst_heat_err);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_disp_memory));
    static const char drw[] = "Disp. R/W";
    pd->tst_disp_memory.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)drw));
    pd->tst_disp_memory.Enable();
    pd->tst_disp_memory.SetTag(STI_tst_disp_memory);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, y, 220, 22), &(pd->tst_stack_overflow));
    static const char so[] = "Stack overflow";
    pd->tst_stack_overflow.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)so));
    pd->tst_stack_overflow.Enable();
    pd->tst_stack_overflow.SetTag(STI_tst_stack_overflow);

    pd->id_tim = gui_timer_create_oneshot(2000, id0);
    pd->id_tim1 = gui_timer_create_periodical(4000, id0);
}

void screen_test_done(screen_t *screen) {
    window_destroy(pd->id);
}

void screen_test_draw(screen_t *screen) {
}

static volatile int _recursive = 1;
static volatile void recursive(uint64_t i) {
    uint64_t x = i + (uint64_t)_recursive;
    osDelay(1);
    if (_recursive)
        recursive(x);
}

int screen_test_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case STI_back:
            screen_close();
            return 1;
        case STI_tst_gui:
            screen_open(get_scr_test_gui()->id);
            return 1;
        case STI_tst_term:
            screen_open(get_scr_test_term()->id);
            return 1;
        case STI_tst_msgbox:
            screen_open(get_scr_test_msgbox()->id);
            return 1;
        case STI_tst_graph:
            screen_open(get_scr_test_graph()->id);
            return 1;
        case STI_tst_temperature:
            screen_open(get_scr_test_temperature()->id);
            return 1;
        case STI_tst_heat_err:
            temp_error("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);
            return 1;
        case STI_tst_disp_memory:
            screen_open(get_scr_test_disp_mem()->id);
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
    0,                          //pdata
};

screen_t *const get_scr_test() { return &screen_test; }
