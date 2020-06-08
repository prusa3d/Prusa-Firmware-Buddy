// screen_test_gui.c

#include "gui.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "screens.h"

typedef struct
{
    window_frame_t frame;
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
} screen_test_gui_data_t;

#define pd ((screen_test_gui_data_t *)screen->pdata)

void screen_test_gui_init(screen_t *screen) {
    int16_t id;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    id = window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(10, 0, 0, 0), &(pd->logo_prusa_mini));
    window_enable(id);
    window_set_tag(id, 10);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 70, 60, 22), &(pd->text0));
    pd->text0.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"Big");

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(80, 70, 60, 22), &(pd->text1));
    pd->text1.font = resource_font(IDR_FNT_NORMAL); // ignore GUI_DEF_FONT
    window_set_text(id, (const char *)"Normal");

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(150, 70, 60, 22), &(pd->text2));
    pd->text2.font = resource_font(IDR_FNT_SMALL);
    window_set_text(id, (const char *)"Small");

    id = window_create_ptr(WINDOW_CLS_NUMB, id0, rect_ui16(10, 100, 60, 22), &(pd->numb0));
    window_set_format(id, (const char *)"%.0f");
    window_set_value(id, 100.0F);

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(80, 100, 60, 22), &(pd->spin0));
    window_set_format(id, "%1.0f");
    window_set_min_max_step(id, 0.0F, 270.0F, 1.0F);
    window_set_value(id, 100.0F);

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(150, 100, 60, 22), &(pd->spin1));
    window_set_format(id, "%.3f");
    window_set_min_max_step(id, 0.0F, 1.0F, 0.001F);
    window_set_value(id, 1.000F);

    id = window_create_ptr(WINDOW_CLS_LIST, id0, rect_ui16(10, 130, 220, 66), &(pd->list));
    window_set_item_index(id, 2);

    id = window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(10, 234, 64, 64), &(pd->icon0));
    window_set_icon_id(id, IDR_PNG_menu_icon_print);
    window_enable(id);
    window_set_tag(id, 1);

    id = window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(80, 234, 64, 64), &(pd->icon1));
    window_set_icon_id(id, IDR_PNG_menu_icon_preheat);
    window_enable(id);
    window_set_tag(id, 2);

    id = window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(150, 234, 64, 64), &(pd->icon2));
    window_set_icon_id(id, IDR_PNG_menu_icon_spool);
    window_enable(id);
    window_set_tag(id, 3);

    id = window_create_ptr(WINDOW_CLS_PROGRESS, id0, rect_ui16(0, 200, 240, 30), &(pd->progress));
    //window_set_icon_id(id, IDR_PNG_menu_icon_spool);
    //window_enable(id);
    //window_set_tag(id, 3);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, 298, 240, 22), &(pd->text_terminal));
    pd->text_terminal.font = resource_font(IDR_FNT_TERMINAL);
    window_set_text(id, (const char *)"Terminal Font IBM ISO9");
}

void screen_test_gui_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
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
    /*	if (event == WINDOW_EVENT_LOOP)
	{
		float temp = window_get_value(screen->pd->spin0.window.win.id);
		int val = sim_heater_temp2val(temp);
		window_set_value(screen->pd->numb0.win.id, val);
	}*/
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
