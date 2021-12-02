/*
 * screen_sysinf.cpp
 *
 *  Created on: 2019-09-25
 *      Author: Radek Vana
 */

#include "screen_sysinf.hpp"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "ScreenHandler.hpp"
#include "sys.h"
#include "../Middlewares/ST/Utilites/CPU/cpu_utils.h"
#include "i18n.h"
#include "marlin_client.h"

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
#define RECT_MACRO(col) Rect16(col_##col, row2draw, col_##col##_w, row_h)

screen_sysinfo_data_t::screen_sysinfo_data_t()
    : AddSuperWindow<screen_t>()
    , textMenuName(this, Rect16(0, 0, display::GetW(), 22), is_multiline::no)
    , textCPU_load(this, Rect16(col_0, 25, col_0_w, row_h), is_multiline::no)
    , textCPU_load_val(this, Rect16(col_1, 25, col_1_w, row_h))
    , textDateTime(this, Rect16(0, 50, display::GetW(), row_h), is_multiline::no)
    , textPrintFan_RPM(this, Rect16(col_0, 75, col_0_w, row_h), is_multiline::no)
    , textPrintFan_RPM_val(this, Rect16(col_1, 75, col_1_w, row_h))
    , textHeatBreakFan_RPM(this, Rect16(col_0, 100, col_0_w, row_h), is_multiline::no)
    , textHeatBreakFan_RPM_val(this, Rect16(col_1, 100, col_1_w, row_h))
    , textExit(this, Rect16(col_0, 290, 60, 22), is_multiline::no, is_closed_on_click_t::yes) {

    textMenuName.font = resource_font(IDR_FNT_BIG);
    static const char dt[] = "System info";
    textMenuName.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)dt));

    //write pattern
    textCPU_load.font = resource_font(IDR_FNT_NORMAL);
    static const char cl[] = N_("CPU load");
    textCPU_load.SetText(_(cl));

    textCPU_load_val.SetFormat((const char *)"%.0f");
    textCPU_load_val.SetValue(osGetCPUUsage());

#ifdef DEBUG_NTP
    time_t sec = sntp_get_system_time();
    struct tm now;
    localtime_r(&sec, &now);
    static char buff[40];
    FormatMsgPrintWillEnd::Date(buff, 40, &now, true, FormatMsgPrintWillEnd::ISO);
    textDateTime.font = resource_font(IDR_FNT_NORMAL);
    textDateTime.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)buff));
#endif

    textPrintFan_RPM.font = resource_font(IDR_FNT_NORMAL);
    static const char cl0[] = N_("PrintFan RPM");
    textPrintFan_RPM.SetText(_(cl0));

    textHeatBreakFan_RPM.font = resource_font(IDR_FNT_NORMAL);
    static const char cl1[] = N_("HB Fan RPM");
    textHeatBreakFan_RPM.SetText(_(cl1));

    textPrintFan_RPM_val.SetFormat((const char *)"%0.0f");
    textPrintFan_RPM_val.SetValue(marlin_vars()->print_fan_rpm);

    textHeatBreakFan_RPM_val.SetFormat((const char *)"%0.0f");
    textHeatBreakFan_RPM_val.SetValue(marlin_vars()->heatbreak_fan_rpm);

    textExit.font = resource_font(IDR_FNT_BIG);

    static const char ex[] = N_("EXIT");
    textExit.SetText(_(ex));
}

void screen_sysinfo_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        actual_CPU_load = osGetCPUUsage();
        if (last_CPU_load != actual_CPU_load) {
            textCPU_load_val.SetValue(actual_CPU_load);
            last_CPU_load = actual_CPU_load;
        }
        if (marlin_change_clr(MARLIN_VAR_PRINT_FAN_RPM))
            textPrintFan_RPM_val.SetValue(marlin_vars()->print_fan_rpm);
        if (marlin_change_clr(MARLIN_VAR_HEATBREAK_FAN_RPM))
            textHeatBreakFan_RPM_val.SetValue(marlin_vars()->heatbreak_fan_rpm);
    }
    SuperWindowEvent(sender, event, param);
}
