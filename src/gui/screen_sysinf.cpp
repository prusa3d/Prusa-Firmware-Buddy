/*
 * screen_sysinf.cpp
 *
 *  Created on: 2019-09-25
 *      Author: Radek Vana
 */

#include "screen_sysinf.hpp"
#include "config.h"
#include "stm32f4xx_hal.h"

#include "sys.h"
#include "../Middlewares/ST/Utilites/CPU/cpu_utils.h"
#include "../lang/i18n.h"

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
enum { row_h = 20 };
#define RECT_MACRO(col) rect_ui16(col_##col, row2draw, col_##col##_w, row_h)

enum {
    TAG_QUIT = 10

};

screen_sysinfo_data_t::screen_sysinfo_data_t()
    : window_frame_t(&textMenuName)
    , textMenuName(this, rect_ui16(0, 0, display::GetW(), 22))
    , textCPU_load(this, rect_ui16(col_0, 25, col_0_w, row_h))
    , textCPU_load_val(this, rect_ui16(col_1, 25, col_1_w, row_h))
    , textExit(this, rect_ui16(col_0, 290, 60, 22)) {

    textMenuName.font = resource_font(IDR_FNT_BIG);
    static const char dt[] = "Disp. TEST rd mem.";
    textMenuName.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)dt));

    //write pattern
    textCPU_load.font = resource_font(IDR_FNT_NORMAL);
    static const char cl[] = N_("CPU load");
    textCPU_load.SetText(_(cl));

    textCPU_load_val.SetFormat((const char *)"%.0f");
    textCPU_load_val.SetValue(osGetCPUUsage());

    textExit.font = resource_font(IDR_FNT_BIG);
    static const char ex[] = N_("EXIT");
    textExit.SetText(_(ex));
    textExit.Enable();
    textExit.SetTag(TAG_QUIT);
}

int screen_sysinfo_data_t::event(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            screen_close();
            return 1;
        }

    if (event == WINDOW_EVENT_LOOP) {
        actual_CPU_load = osGetCPUUsage();
        if (last_CPU_load != actual_CPU_load) {
            textCPU_load_val.SetValue(actual_CPU_load);
            last_CPU_load = actual_CPU_load;
        }
    }

    return 0;
}
