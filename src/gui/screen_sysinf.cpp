/*
 * screen_sysinf.cpp
 *
 *  Created on: 2019-09-25
 *      Author: Radek Vana
 */

#include "gui.hpp"
#include "screens.h"
#include "config.h"
#include "stm32f4xx_hal.h"

#include "sys.h"
#include "../Middlewares/ST/Utilites/CPU/cpu_utils.h"

#include "../lang/i18n.h"

// Use this #define to hide the static display of current NTP time - only for debugging
// Clean solution will come later
//#define DEBUG_NTP

#ifdef DEBUG_NTP
    #include "../lang/format_print_will_end.hpp"
    #include "wui_api.h"
#endif

struct screen_sysinfo_data_t {
    window_frame_t frame;
    window_text_t textMenuName;
    window_text_t textCPU_load;
    window_numb_t textCPU_load_val;
#ifdef DEBUG_NTP
    window_text_t textDateTime;
#endif

    window_text_t textExit;
};

#define pd ((screen_sysinfo_data_t *)screen->pdata)
/******************************************************************************************************/
//variables

static int actual_CPU_load = -1;
static int last_CPU_load = -1;

/******************************************************************************************************/
//methods

/******************************************************************************************************/
//column specifications
enum { col_0 = 2,
    col_1 = 96 };
enum { col_0_w = col_1 - col_0,
    col_1_w = 240 - col_1 - col_0 };
enum { col_2_w = 38 };
#define RECT_MACRO(col) rect_ui16(col_##col, row2draw, col_##col##_w, row_h)

enum {
    TAG_QUIT = 10

};

void screen_sysinfo_init(screen_t *screen) {
    int16_t row2draw = 0;
    int16_t row_h = 20;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, 0, display::GetW(), 22), &(pd->textMenuName));
    pd->textMenuName.font = resource_font(IDR_FNT_BIG);
    static const char dt[] = "Disp. TEST rd mem.";
    pd->textMenuName.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)dt));

    row2draw += 25;

    //write pattern
    window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textCPU_load));
    pd->textCPU_load.font = resource_font(IDR_FNT_NORMAL);
    static const char cl[] = N_("CPU load");
    pd->textCPU_load.SetText(_(cl));

    window_create_ptr(WINDOW_CLS_NUMB, id0, RECT_MACRO(1), &(pd->textCPU_load_val));
    pd->textCPU_load_val.SetFormat((const char *)"%.0f");
    pd->textCPU_load_val.SetValue(osGetCPUUsage());

    row2draw += 25;

#ifdef DEBUG_NTP
    time_t sec = sntp_get_system_time();
    struct tm now;
    localtime_r(&sec, &now);
    static char buff[40];
    FormatMsgPrintWillEnd::Date(buff, 40, &now, true, FormatMsgPrintWillEnd::ISO);
    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, row2draw, display::GetW(), 22), &(pd->textDateTime));
    pd->textDateTime.font = resource_font(IDR_FNT_NORMAL);
    pd->textDateTime.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)buff));

    row2draw += 25;
#endif

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col_0, 290, 60, 22), &(pd->textExit));
    pd->textExit.font = resource_font(IDR_FNT_BIG);
    static const char ex[] = N_("EXIT");
    pd->textExit.SetText(_(ex));
    pd->textExit.Enable();
    pd->textExit.SetTag(TAG_QUIT);
}

void screen_sysinfo_done(screen_t *screen) {
    window_destroy(pd->frame.id);
}

void screen_sysinfo_draw(screen_t *screen) {
}

int screen_sysinfo_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            screen_close();
            return 1;
        }

    if (event == WINDOW_EVENT_LOOP) {
        actual_CPU_load = osGetCPUUsage();
        if (last_CPU_load != actual_CPU_load) {
            pd->textCPU_load_val.SetValue(actual_CPU_load);
            last_CPU_load = actual_CPU_load;
        }
    }

    return 0;
}

screen_t screen_sysinfo = {
    0,
    0,
    screen_sysinfo_init,
    screen_sysinfo_done,
    screen_sysinfo_draw,
    screen_sysinfo_event,
    sizeof(screen_sysinfo_data_t), //data_size
    0,                             //pdata
};

screen_t *const get_scr_sysinfo() { return &screen_sysinfo; }
