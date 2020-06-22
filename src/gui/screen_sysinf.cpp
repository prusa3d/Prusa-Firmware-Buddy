/*
 * screen_sysinf.c
 *
 *  Created on: 2019-09-25
 *      Author: Radek Vana
 */

#include "gui.h"
#include "screens.h"
#include "config.h"
#include "stm32f4xx_hal.h"

#include "sys.h"
#include "../Middlewares/ST/Utilites/CPU/cpu_utils.h"

typedef struct
{
    window_frame_t frame;
    window_text_t textMenuName;
    window_text_t textCPU_load;
    window_numb_t textCPU_load_val;

    window_text_t textExit;
} screen_sysinfo_data_t;

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
    int16_t id;
    int16_t row_h = 20;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, 0, display::GetW(), 22), &(pd->textMenuName));
    pd->textMenuName.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"Disp. TEST rd mem.");

    row2draw += 25;

    //write pattern
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textCPU_load));
    pd->textCPU_load.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"CPU load");

    id = window_create_ptr(WINDOW_CLS_NUMB, id0, RECT_MACRO(1), &(pd->textCPU_load_val));
    window_set_format(id, (const char *)"%.0f");
    window_set_value(id, osGetCPUUsage());

    row2draw += 25;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col_0, 290, 60, 22), &(pd->textExit));
    pd->textExit.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"EXIT");
    window_enable(id);
    window_set_tag(id, TAG_QUIT);
}

void screen_sysinfo_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
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
            window_set_value(pd->textCPU_load_val.win.id, actual_CPU_load);
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
