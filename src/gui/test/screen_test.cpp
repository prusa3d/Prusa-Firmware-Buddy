// screen_test.cpp

#include "screen_test.hpp"
#include "config.h"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "bsod.h"

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

screen_test_data_t::screen_test_data_t()
    : window_frame_t(&test)
    , test(this, rect_ui16(10, 32, 220, 22))
    , back(this, rect_ui16(10, 54, 220, 22))
    , tst_gui(this, rect_ui16(10, 76, 220, 22))
    , tst_term(this, rect_ui16(10, 98, 220, 22))
    , tst_msgbox(this, rect_ui16(10, 120, 220, 22))
    , tst_graph(this, rect_ui16(10, 142, 220, 22))
    , tst_temperature(this, rect_ui16(10, 164, 220, 22))
    , tst_heat_err(this, rect_ui16(10, 186, 220, 22))
    , tst_disp_memory(this, rect_ui16(10, 208, 220, 22))
    , tst_stack_overflow(this, rect_ui16(10, 230, 220, 22))
    , id_tim(gui_timer_create_oneshot(2000, 0))  //id0
    , id_tim1(gui_timer_create_oneshot(2000, 0)) //id0
{
    static const char tst[] = "TEST";
    test.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tst));

    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));
    back.Enable();
    back.SetTag(STI_back);

    static const char tstg[] = "test GUI";
    tst_gui.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstg));
    tst_gui.Enable();
    tst_gui.SetTag(STI_tst_gui);

    static const char tstt[] = "test TERM";
    tst_term.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstt));
    tst_term.Enable();
    tst_term.SetTag(STI_tst_term);

    static const char tstm[] = "test MSGBOX";
    tst_msgbox.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstm));
    tst_msgbox.Enable();
    tst_msgbox.SetTag(STI_tst_msgbox);

    static const char tmpg[] = "temp graph";
    tst_graph.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tmpg));
    tst_graph.Enable();
    tst_graph.SetTag(STI_tst_graph);

    static const char tmpp[] = "temp - pwm";
    tst_temperature.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tmpp));
    tst_temperature.Enable();
    tst_temperature.SetTag(STI_tst_temperature);

    static const char he[] = "HEAT ERROR";
    tst_heat_err.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)he));
    tst_heat_err.Enable();
    tst_heat_err.SetTag(STI_tst_heat_err);

    static const char drw[] = "Disp. R/W";
    tst_disp_memory.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)drw));
    tst_disp_memory.Enable();
    tst_disp_memory.SetTag(STI_tst_disp_memory);

    static const char so[] = "Stack overflow";
    tst_stack_overflow.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)so));
    tst_stack_overflow.Enable();
    tst_stack_overflow.SetTag(STI_tst_stack_overflow);
}

static volatile int _recursive = 1;
static volatile void recursive(uint64_t i) {
    uint64_t x = i + (uint64_t)_recursive;
    osDelay(1);
    if (_recursive)
        recursive(x);
}

int screen_test_data_t::event(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case STI_back:
            screen_close();
            return 1;
        case STI_tst_gui:
            // screen_open(get_scr_test_gui()->id);
            return 1;
        case STI_tst_term:
            //screen_open(get_scr_test_term()->id);
            return 1;
        case STI_tst_msgbox:
            //screen_open(get_scr_test_msgbox()->id);
            return 1;
        case STI_tst_graph:
            //screen_open(get_scr_test_graph()->id);
            return 1;
        case STI_tst_temperature:
            //screen_open(get_scr_test_temperature()->id);
            return 1;
        case STI_tst_heat_err:
            ("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);
            return 1;
        case STI_tst_disp_memory:
            //screen_open(get_scr_test_disp_mem()->id);
            return 1;
        case STI_tst_stack_overflow:
            recursive(0);
            return 1;
        }
    /*else if (event == WINDOW_EVENT_TIMER) {
        if ((int)param == id_tim)
            _dbg("tim0 %lu", HAL_GetTick());
        else if ((int)param == id_tim1)
            _dbg("tim1 %lu", HAL_GetTick());
        return 1;
    }*/
    return 0;
}
