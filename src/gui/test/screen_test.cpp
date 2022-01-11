// screen_test.cpp

#include "screen_test.hpp"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "bsod.h"
#include "ScreenHandler.hpp"
#include "screen_test_gui.hpp"
#include "screen_test_term.hpp"
#include "screen_test_msgbox.hpp"
#include "screen_test_wizard_icons.hpp"
#include "screen_test_dlg.hpp"
#include "screen_menu_eeprom_test.hpp"

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
    , test(this, Rect16(10, 32, 220, 22), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)"TEST"))
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes, string_view_utf8::MakeCPUFLASH((const uint8_t *)"back"))
    , tst_eeprom(
          this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(GetScreenMenuEepromTest); }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"test EEPROM"))
    , tst_gui(
          this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_gui_data_t>); }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"test GUI"))
    , tst_term(
          this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_term_data_t>); }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"test TERM"))
    , tst_msgbox(
          this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_msgbox_data_t>); }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"test MSGBOX"))
    , tst_wizard_icons(
          this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_wizard_icons>); }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"test Wizard icons"))
    , tst_safety_dlg(
          this, this->GenerateRect(ShiftDir_t::Bottom), []() { Screens::Access()->Open(ScreenFactory::Screen<screen_test_dlg_data_t>); }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"test dialog"))
#if 0
    , tst_graph(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_graph()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp graph"))
    , tst_temperature(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_temperature()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp - pwm"))
    , tst_heat_err(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"HEAT ERROR"))
    , tst_disp_memory(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_disp_mem()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"Disp. R/W"))
#endif // 0
    , tst_stack_overflow(
          this, this->GenerateRect(ShiftDir_t::Bottom), []() { recursive(0); }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"Stack overflow"))
    , tst_stack_div0(
          this, this->GenerateRect(ShiftDir_t::Bottom), []() {
              static volatile int i = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
              i = i / 0;
#pragma GCC diagnostic pop
          },
          string_view_utf8::MakeCPUFLASH((const uint8_t *)"BSOD div 0"))
    , id_tim(gui_timer_create_oneshot(this, 2000))  //id0
    , id_tim1(gui_timer_create_oneshot(this, 2000)) //id0
{
}
