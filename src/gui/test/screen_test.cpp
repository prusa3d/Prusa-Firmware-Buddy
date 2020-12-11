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
#include "screen_test_wizard_icons.hpp"

//fererate stack overflow
static volatile int _recursive = 1;
static volatile void recursive(uint64_t i) {
    uint64_t x = i + (uint64_t)_recursive;
    osDelay(1);
    if (_recursive)
        recursive(x);
}

screen_test_data_t::screen_test_data_t()
    : AddSuperWindow<screen_t>()
    , test(this, Rect16(10, 32, 220, 22), is_multiline::no)
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes)
    , tst_gui(this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_gui_data_t>); })
    , tst_term(this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_term_data_t>); })
    , tst_msgbox(this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_msgbox_data_t>); })
    , tst_wizard_icons(this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_wizard_icons>); })
    , tst_graph(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_graph()->id);*/ })
    , tst_temperature(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_temperature()->id);*/ })
    , tst_heat_err(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);*/ })
    , tst_disp_memory(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_disp_mem()->id);*/ })
    , tst_stack_overflow(this, this->GenerateRect(ShiftDir_t::Bottom), []() { recursive(0); })
    , id_tim(gui_timer_create_oneshot(this, 2000))  //id0
    , id_tim1(gui_timer_create_oneshot(this, 2000)) //id0
{
    static const char tst[] = "TEST";
    test.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tst));

    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));

    static const char tstg[] = "test GUI";
    tst_gui.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstg));

    static const char tstt[] = "test TERM";
    tst_term.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstt));

    static const char tstm[] = "test MSGBOX";
    tst_msgbox.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tstm));

    static const char tswi[] = "test Wizard icons";
    tst_wizard_icons.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tswi));

    static const char tmpg[] = "temp graph";
    tst_graph.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tmpg));

    static const char tmpp[] = "temp - pwm";
    tst_temperature.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tmpp));

    static const char he[] = "HEAT ERROR";
    tst_heat_err.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)he));

    static const char drw[] = "Disp. R/W";
    tst_disp_memory.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)drw));

    static const char so[] = "Stack overflow";
    tst_stack_overflow.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)so));
}
