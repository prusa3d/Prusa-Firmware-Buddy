// screen_watchdog.cpp

#include "gui.hpp"
#include "config.h"

struct screen_watchdog_data_t : public window_frame_t {
    window_text_t text0;
    window_text_t text1;
};

struct screen_watchdog_t {
    screen_t scr;
    screen_watchdog_data_t *pd;
};

void screen_watchdog_init(screen_watchdog_t *screen) {
    if (screen->pd == 0) {
        int16_t id0;
        screen_watchdog_data_t *pd = (screen_watchdog_data_t *)gui_malloc(sizeof(screen_watchdog_data_t));
        screen->pd = pd;

        id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), pd);
        pd->SetBackColor(COLOR_RED);

        window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 70, 220, 24), &(pd->text0));
        pd->text0.font = resource_font(IDR_FNT_BIG);
        static const char wdgr[] = "WATCHDOG RESET";
        pd->text0.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wdgr));
        pd->text0.SetAlignment(ALIGN_CENTER);

        window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, 110, 240, 24), &(pd->text1));
        pd->text1.font = resource_font(IDR_FNT_NORMAL);
        static const char ptc[] = "press to continue...";
        pd->text1.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ptc));
        pd->text1.SetAlignment(ALIGN_CENTER);
        pd->text1.Enable();
        pd->text1.SetTag(1);
    }
}

void screen_watchdog_done(screen_watchdog_t *screen) {
    if (screen->pd) {
        window_destroy(screen->pd->id);
        gui_free(screen->pd);
        screen->pd = 0;
    }
}

void screen_watchdog_draw(screen_watchdog_t *screen) {
}

int screen_watchdog_event(screen_watchdog_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        screen_close();
    return 0;
}
