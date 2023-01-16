// screen_test_gui.cpp

#include "screen_test_gui.hpp"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "ScreenHandler.hpp"
#include "png_resources.hpp"

screen_test_gui_data_t::screen_test_gui_data_t()
    : AddSuperWindow<screen_t>()
    , png_printer("/internal/res/printer_logo.png") // dimensions are printer dependent
    , logo_prusa_printer(this, Rect16(0, 84, 240, 62), &png_printer, is_closed_on_click_t::yes)
    , text0(this, Rect16(10, 70, 60, 22), is_multiline::no)
    , text1(this, Rect16(80, 70, 60, 22), is_multiline::no)
    , text2(this, Rect16(150, 70, 60, 22), is_multiline::no)
    , numb0(this, Rect16(10, 100, 60, 22))
    , icon0(this, Rect16(10, 234, 64, 64), &png::print_58x58)
    , icon1(this, Rect16(80, 234, 64, 64), &png::preheat_58x58)
    , icon2(this, Rect16(150, 234, 64, 64), &png::spool_58x58)
    , progress(this, Rect16(0, 200, 240, 30))
    , text_terminal(this, Rect16(0, 298, 240, 22), is_multiline::no) {

    text0.font = resource_font(IDR_FNT_BIG);
    static const char big[] = "Big";
    text0.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)big));

    text1.font = resource_font(IDR_FNT_NORMAL); // ignore GUI_DEF_FONT
    static const char nrm[] = "Normal";
    text1.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)nrm));

    text2.font = resource_font(IDR_FNT_SMALL);
    static const char sml[] = "Small";
    text2.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)sml));

    numb0.SetFormat((const char *)"%.0f");
    numb0.SetValue(100.0F);

    text_terminal.font = resource_font(IDR_FNT_TERMINAL);
    static const char tf[] = "Terminal Font IBM ISO9";
    text_terminal.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tf));
}
