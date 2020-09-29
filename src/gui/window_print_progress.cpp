// window_print_progress.cpp
#include "window_print_progress.hpp"
#include "gui.hpp"
#include <algorithm>

/*****************************************************************************/
//WindowPrintProgress
#include "marlin_client.h"
WindowPrintProgress::WindowPrintProgress(window_t *parent, point_i16_t position)
    : AddSuperWindow<window_progress_t>(parent, Rect16(position.x, position.y, GuiDefaults::RectScreen.Width() - position.x, 50), 16, COLOR_ORANGE)
    , last_sd_percent_done(-1) {
    SetFont(resource_font(IDR_FNT_BIG));
}

void WindowPrintProgress::update_progress(uint8_t percent, uint16_t print_speed) {
    SetNumbColor((percent <= 100) && (print_speed == 100) ? GuiDefaults::COLOR_VALUE_VALID : GuiDefaults::COLOR_VALUE_INVALID);
    SetValue(percent);
}

void WindowPrintProgress::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        if (marlin_vars()->sd_percent_done != last_sd_percent_done) {
            update_progress(marlin_vars()->sd_percent_done, marlin_vars()->print_speed);
            last_sd_percent_done = marlin_vars()->sd_percent_done;
        }
    }
    SuperWindowEvent(sender, event, param);
}
