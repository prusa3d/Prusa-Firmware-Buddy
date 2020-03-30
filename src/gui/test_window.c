// test_window.c

#include "cmsis_os.h"
#include "config.h"
#include "gui.h"
#include "window.h"
#include "jogwheel.h"

extern osThreadId displayTaskHandle;
/*
void ui_loop(void)
{
//	int count = window_enabled_child_count(0);
//	int16_t id = window_first_child(0);
//	if (!window_is_enabled(id))
//		id = window_next_enabled(id);
//	window_set_focus(id);
//	jogwheel_encoder_set(0, 0, count-1);
	window_set_capture(0);
	int old_enc = jogwheel_encoder;
	int old_btn = jogwheel_button_down;
	window_invalidate(0);
	while (1)
	{
//		if (jogwheel_button_down)
//			jogwheel_encoder = old_enc;
		if ((jogwheel_encoder != old_enc))
		{
			int dif = jogwheel_encoder - old_enc;
			if (dif > 0)
				window_dispatch_event(WINDOW_EVENT_ENC_UP, (void*)dif);
			else if (dif < 0)
				window_dispatch_event(WINDOW_EVENT_ENC_DN, (void*)-dif);
			//while (dif > 0) { id = window_next_enabled(id); dif--; }
			//while (dif < 0) { id = window_prev_enabled(id); dif++; }
			//window_set_focus(id);
			old_enc = jogwheel_encoder;
		}
		if (!jogwheel_button_down ^ !old_btn)
		{
			if (jogwheel_button_down)
				window_dispatch_event(WINDOW_EVENT_BTN_DN, 0);
			else
				window_dispatch_event(WINDOW_EVENT_BTN_UP, 0);
			old_btn = jogwheel_button_down;
		}
		if (osSignalWait(SIG_DISP_REDRAW, 50).status == osEventSignal)
			window_draw(0);
	}
}
*/

#include "screen.h"

void test_window(void) {
    //	get_scr_test()->init(get_scr_test());
    //	get_scr_test()->init(get_scr_test());
    /*
	font_t* font = resource_font(IDR_FNT_11x18_4);
	int16_t id0 = window_frame_create(-1, rect_ui16(0,0,0,0), COLOR_GRAY);
	int16_t id1 = window_icon_create(id0, rect_ui16(10, 0, 0, 0), IDR_PNG_splash_logo_prusamini, COLOR_BLACK);
	int16_t id2 = window_text_create(id0, rect_ui16(10, 70, 60, 22), "Test0", font, COLOR_BLACK, COLOR_WHITE, padding_ui8(2, 2, 2, 2), ALIGN_LEFT_CENTER);
	int16_t id3 = window_text_create(id0, rect_ui16(80, 70, 60, 22), "Test1", font, COLOR_BLACK, COLOR_WHITE, padding_ui8(2, 2, 2, 2), ALIGN_LEFT_CENTER);
	int16_t id4 = window_numb_create(id0, rect_ui16(10, 100, 60, 22), 10.0F, "%1.0f", font, COLOR_BLACK, COLOR_WHITE, padding_ui8(2, 2, 2, 2), ALIGN_LEFT_CENTER);
	int16_t id5 = window_numb_create(id0, rect_ui16(80, 100, 60, 22), 1.500F, "%.3f", font, COLOR_BLACK, COLOR_WHITE, padding_ui8(2, 2, 2, 2), ALIGN_LEFT_CENTER);
	int16_t id6 = window_spin_create_min_max_step(id0, rect_ui16(10, 130, 60, 22), 100.0F, "%1.0f", font, COLOR_BLACK, COLOR_WHITE, padding_ui8(2, 2, 2, 2), ALIGN_LEFT_CENTER, 0.0F, 100.0F, 1.0F);
	int16_t id7 = window_spin_create_min_max_step(id0, rect_ui16(80, 130, 60, 22), 1.000F, "%.3f", font, COLOR_BLACK, COLOR_WHITE, padding_ui8(2, 2, 2, 2), ALIGN_LEFT_CENTER, 0.0F, 1.0F, 0.001F);
	int16_t id8 = window_list_create(id0, rect_ui16(10, 160, 220, 88), font, COLOR_BLACK, COLOR_WHITE, padding_ui8(18, 2, 2, 2), ALIGN_LEFT_CENTER, 0);
	int16_t id9 = window_icon_create(id0, rect_ui16(10, 250, 64, 64), IDR_PNG_menu_icon_print, COLOR_BLACK);
	int16_t id10 = window_icon_create(id0, rect_ui16(80, 250, 64, 64), IDR_PNG_menu_icon_preheat, COLOR_BLACK);
	int16_t id11 = window_icon_create(id0, rect_ui16(150, 250, 64, 64), IDR_PNG_menu_icon_spool, COLOR_BLACK);
	//int16_t id12 = window_icon_create(id0, rect_ui16(10, 250, 64, 64), IDR_PNG_menu_icon_print, COLOR_BLACK, ALIGN_CENTER);
	window_enable(id1);
	window_enable(id2);
	window_enable(id3);
	window_enable(id4);
	window_enable(id5);
	window_enable(id8);
	window_enable(id9);
	window_enable(id10);
	window_enable(id11);
	//window_enable(id12);*/
    //	ui_loop();
    while (1)
        gui_loop();
}
