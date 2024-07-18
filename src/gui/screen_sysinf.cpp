/*
 * screen_sysinf.cpp
 *
 *  Created on: 2019-09-25
 *      Author: Radek Vana
 */

#include "screen_sysinf.hpp"
#include "config.h"
#include "sound.hpp"
#include "stm32f4xx_hal.h"
#include "ScreenHandler.hpp"
#include "sys.h"
#include "../hw/cpu_utils.hpp"
#include "i18n.h"
#include "marlin_client.hpp"
#include <ctime>

/******************************************************************************************************/
// variables

static int last_CPU_load = -1;

/******************************************************************************************************/
// methods

/******************************************************************************************************/
// column specifications
enum { col_0 = 2,
    col_1 = 96 };
enum { col_0_w = col_1 - col_0,
    col_1_w = 240 - col_1 - col_0 };
enum { col_2_w = 38 };
enum { row_h = 20 };
#define RECT_MACRO(col) Rect16(col_##col, row2draw, col_##col##_w, row_h)

screen_sysinfo_data_t::screen_sysinfo_data_t()
    : screen_t()
    , textMenuName(this, Rect16(0, 0, GuiDefaults::ScreenWidth, 22), is_multiline::no)
    , textCPU_load(this, Rect16(col_0, 25, col_0_w, row_h), is_multiline::no)
    , textCPU_load_val(this, Rect16(col_1, 25, col_1_w, row_h))
    , textDateTime(this, Rect16(0, 50, GuiDefaults::ScreenWidth, row_h), is_multiline::no)
    , textPrintFan_RPM(this, Rect16(col_0, 75, col_0_w, row_h), is_multiline::no)
    , textPrintFan_RPM_val(this, Rect16(col_1, 75, col_1_w, row_h))
    , textHeatBreakFan_RPM(this, Rect16(col_0, 100, col_0_w, row_h), is_multiline::no)
    , textHeatBreakFan_RPM_val(this, Rect16(col_1, 100, col_1_w, row_h))
    , textExit(this, Rect16(col_0, 290, 60, 22), is_multiline::no, is_closed_on_click_t::yes) {

    textMenuName.set_font(Font::big);
    static const char dt[] = "System info";
    textMenuName.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)dt));

    // write pattern
    textCPU_load.set_font(Font::normal);
    textCPU_load.SetText(_("CPU load"));

    textCPU_load_val.SetFormat((const char *)"%.0f");
    textCPU_load_val.SetValue(osGetCPUUsage());

#ifdef DEBUG_NTP
    time_t sec = time(nullptr);
    if (sec == (time_t)-1) {
        sec = 0;
    }
    struct tm now;
    localtime_r(&sec, &now);
    static char buff[40];
    FormatMsgPrintWillEnd::Date(buff, 40, &now, true, FormatMsgPrintWillEnd::ISO);
    textDateTime.set_font(Font::normal);
    textDateTime.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)buff));
#endif

    textPrintFan_RPM.set_font(Font::normal);
    textPrintFan_RPM.SetText(_("PrintFan RPM"));

    textHeatBreakFan_RPM.set_font(Font::normal);
    textHeatBreakFan_RPM.SetText(_("HB Fan RPM"));

    textPrintFan_RPM_val.SetFormat((const char *)"%0.0f");
    textPrintFan_RPM_val.SetValue(marlin_vars().active_hotend().print_fan_rpm);

    textHeatBreakFan_RPM_val.SetFormat((const char *)"%0.0f");
    textHeatBreakFan_RPM_val.SetValue(marlin_vars().active_hotend().heatbreak_fan_rpm);

    textExit.set_font(Font::big);
    textExit.SetText(_("EXIT"));
}

void screen_sysinfo_data_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT: {
        Sound_Play(eSOUND_TYPE::ButtonEcho);
        Screens::Access()->Close();
        return;
    }

    case GUI_event_t::LOOP: {
        const int actual_CPU_load = osGetCPUUsage();
        if (last_CPU_load != actual_CPU_load) {
            textCPU_load_val.SetValue(actual_CPU_load);
            last_CPU_load = actual_CPU_load;
        }

        textPrintFan_RPM_val.SetValue(marlin_vars().active_hotend().print_fan_rpm);
        textHeatBreakFan_RPM_val.SetValue(marlin_vars().active_hotend().heatbreak_fan_rpm);
        break;
    }

    default:
        break;
    }

    screen_t::windowEvent(sender, event, param);
}
