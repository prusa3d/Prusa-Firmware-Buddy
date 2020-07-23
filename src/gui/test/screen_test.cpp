// screen_test.cpp

#include "screen_test.hpp"
#include "config.h"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "bsod.h"
#include "ScreenHandler.hpp"
#include "screen_test_gui.hpp"
#include "screen_test_term.hpp"
#include "screen_test_msgbox.hpp"

//fererate stack overflow
static volatile int _recursive = 1;
static volatile void recursive(uint64_t i) {
    uint64_t x = i + (uint64_t)_recursive;
    osDelay(1);
    if (_recursive)
        recursive(x);
}

screen_test_data_t::screen_test_data_t()
    : window_frame_t(&test)
    , test(this, rect_ui16(10, 32, 220, 22))
    , back(this, rect_ui16(10, 54, 220, 22), []() { Screens::Access()->Close(); })
    , tst_gui(this, rect_ui16(10, 76, 220, 22), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_gui_data_t>); })
    , tst_term(this, rect_ui16(10, 98, 220, 22), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_term_data_t>); })
    , tst_msgbox(this, rect_ui16(10, 120, 220, 22), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_msgbox_data_t>); })
    , tst_graph(this, rect_ui16(10, 142, 220, 22), []() { /*screen_open(get_scr_test_graph()->id);*/ })
    , tst_temperature(this, rect_ui16(10, 164, 220, 22), []() { /*screen_open(get_scr_test_temperature()->id);*/ })
    , tst_heat_err(this, rect_ui16(10, 186, 220, 22), []() { /*("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);*/ })
    , tst_disp_memory(this, rect_ui16(10, 208, 220, 22), []() { /*screen_open(get_scr_test_disp_mem()->id);*/ })
    , tst_stack_overflow(this, rect_ui16(10, 230, 220, 22), []() { recursive(0); })
    , id_tim(gui_timer_create_oneshot(this, 2000))  //id0
    , id_tim1(gui_timer_create_oneshot(this, 2000)) //id0
{
    static const char tst[] = "TEST";
    test.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tst));

    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));
    back.Enable();

    static const char tstg[] = "test GUI";
    tst_gui.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstg));
    tst_gui.Enable();

    static const char tstt[] = "test TERM";
    tst_term.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstt));
    tst_term.Enable();

    static const char tstm[] = "test MSGBOX";
    tst_msgbox.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstm));
    tst_msgbox.Enable();

    static const char tmpg[] = "temp graph";
    tst_graph.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tmpg));
    tst_graph.Enable();

    static const char tmpp[] = "temp - pwm";
    tst_temperature.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tmpp));
    tst_temperature.Enable();

    static const char he[] = "HEAT ERROR";
    tst_heat_err.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)he));
    tst_heat_err.Enable();

    static const char drw[] = "Disp. R/W";
    tst_disp_memory.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)drw));
    tst_disp_memory.Enable();

    static const char so[] = "Stack overflow";
    tst_stack_overflow.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)so));
    tst_stack_overflow.Enable();
}
