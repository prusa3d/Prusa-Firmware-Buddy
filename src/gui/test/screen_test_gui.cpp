// screen_test_gui.cpp

#include "screen_test_gui.hpp"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "ScreenHandler.hpp"

screen_test_gui_data_t::screen_test_gui_data_t()
    : window_frame_t(&logo_prusa_mini)
    , logo_prusa_mini(this, rect_ui16(0, 84, 240, 62), IDR_PNG_splash_logo_prusa_prn)
    , text0(this, rect_ui16(10, 70, 60, 22))
    , text1(this, rect_ui16(80, 70, 60, 22))
    , text2(this, rect_ui16(150, 70, 60, 22))
    , numb0(this, rect_ui16(10, 100, 60, 22))
    , spin0(this, rect_ui16(80, 100, 60, 22))
    , spin1(this, rect_ui16(150, 100, 60, 22))
    , list(this, rect_ui16(10, 130, 220, 66))
    , icon0(this, rect_ui16(10, 234, 64, 64), IDR_PNG_menu_icon_print)
    , icon1(this, rect_ui16(80, 234, 64, 64), IDR_PNG_menu_icon_preheat)
    , icon2(this, rect_ui16(150, 234, 64, 64), IDR_PNG_menu_icon_spool)
    , progress(this, rect_ui16(0, 200, 240, 30))
    , text_terminal(this, rect_ui16(0, 298, 240, 22)) {
    logo_prusa_mini.Enable();
    logo_prusa_mini.SetTag(10);

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

    spin0.SetFormat("%1.0f");
    spin0.SetMinMaxStep(0.0F, 270.0F, 1.0F);
    spin0.SetValue(100.0F);

    spin1.SetFormat("%.3f");
    spin1.SetMinMaxStep(0.0F, 1.0F, 0.001F);
    spin1.SetValue(1.000F);

    list.SetItemIndex(2);

    icon0.Enable();
    icon0.SetTag(1);

    icon1.Enable();
    icon1.SetTag(2);

    icon2.Enable();
    icon2.SetTag(3);

    text_terminal.font = resource_font(IDR_FNT_TERMINAL);
    static const char tf[] = "Terminal Font IBM ISO9";
    text_terminal.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tf));
}

int screen_test_gui_data_t::event(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case 10:
            Screens::Access()->Close();
            return 1;
        }
    return 0;
}
