// screen_test_gui.cpp

#include "gui.hpp"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "screens.h"

struct screen_test_gui_data_t : public window_frame_t {
    window_icon_t logo_prusa_mini;
    window_text_t text0;
    window_text_t text1;
    window_text_t text2;
    window_numb_t numb0;
    window_numb_t numb1;
    window_spin_t spin0;
    window_spin_t spin1;
    window_list_t list;
    window_icon_t icon0;
    window_icon_t icon1;
    window_icon_t icon2;
    window_progress_t progress;
    window_text_t text_terminal;
};

#define pd ((screen_test_gui_data_t *)screen->pdata)

void screen_test_gui_init(screen_t *screen) {
    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), pd);

    window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(10, 0, 0, 0), &(pd->logo_prusa_mini));
    pd->logo_prusa_mini.Enable();
    pd->logo_prusa_mini.SetTag(10);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 70, 60, 22), &(pd->text0));
    pd->text0.font = resource_font(IDR_FNT_BIG);
    static const char big[] = "Big";
    pd->text0.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)big));

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(80, 70, 60, 22), &(pd->text1));
    pd->text1.font = resource_font(IDR_FNT_NORMAL); // ignore GUI_DEF_FONT
    static const char nrm[] = "Normal";
    pd->text1.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)nrm));

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(150, 70, 60, 22), &(pd->text2));
    pd->text2.font = resource_font(IDR_FNT_SMALL);
    static const char sml[] = "Small";
    pd->text2.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)sml));

    window_create_ptr(WINDOW_CLS_NUMB, id0, rect_ui16(10, 100, 60, 22), &(pd->numb0));
    pd->numb0.SetFormat((const char *)"%.0f");
    pd->numb0.SetValue(100.0F);

    window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(80, 100, 60, 22), &(pd->spin0));
    pd->spin0.SetFormat("%1.0f");
    pd->spin0.SetMinMaxStep(0.0F, 270.0F, 1.0F);
    pd->spin0.SetValue(100.0F);

    window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(150, 100, 60, 22), &(pd->spin1));
    pd->spin1.SetFormat("%.3f");
    pd->spin1.SetMinMaxStep(0.0F, 1.0F, 0.001F);
    pd->spin1.SetValue(1.000F);

    window_create_ptr(WINDOW_CLS_LIST, id0, rect_ui16(10, 130, 220, 66), &(pd->list));
    pd->list.SetItemIndex(2);

    window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(10, 234, 64, 64), &(pd->icon0));
    pd->icon0.SetIdRes(IDR_PNG_menu_icon_print);
    pd->icon0.Enable();
    pd->icon0.SetTag(1);

    window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(80, 234, 64, 64), &(pd->icon1));
    pd->icon1.SetIdRes(IDR_PNG_menu_icon_preheat);
    pd->icon1.Enable();
    pd->icon1.SetTag(2);

    window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(150, 234, 64, 64), &(pd->icon2));
    pd->icon2.SetIdRes(IDR_PNG_menu_icon_spool);
    pd->icon2.Enable();
    pd->icon2.SetTag(3);

    window_create_ptr(WINDOW_CLS_PROGRESS, id0, rect_ui16(0, 200, 240, 30), &(pd->progress));

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, 298, 240, 22), &(pd->text_terminal));
    pd->text_terminal.font = resource_font(IDR_FNT_TERMINAL);
    static const char tf[] = "Terminal Font IBM ISO9";
    pd->text_terminal.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tf));
}

void screen_test_gui_done(screen_t *screen) {
    window_destroy(pd->id);
}

void screen_test_gui_draw(screen_t *screen) {
}

int screen_test_gui_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case 10:
            screen_close();
            return 1;
        }
    return 0;
}

screen_t screen_test_gui = {
    0,
    0,
    screen_test_gui_init,
    screen_test_gui_done,
    screen_test_gui_draw,
    screen_test_gui_event,
    sizeof(screen_test_gui_data_t), //data_size
    0,                              //pdata
};

screen_t *const get_scr_test_gui() { return &screen_test_gui; }
