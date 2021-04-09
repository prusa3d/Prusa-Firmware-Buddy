// window_print_progress.cpp
#include "window_print_progress.hpp"
#include "gui.hpp"
#include <algorithm>
#include "resource.h"

/*****************************************************************************/
//WindowPrintProgress
#include "marlin_client.h"
WindowPrintProgress::WindowPrintProgress(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_numberless_progress_t>(parent, rect)
    , last_sd_percent_done(-1) {
    SetColor(COLOR_ORANGE);
}

void WindowPrintProgress::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        if (marlin_vars()->sd_percent_done != last_sd_percent_done) {
            SetProgressPercent(marlin_vars()->sd_percent_done);
            last_sd_percent_done = marlin_vars()->sd_percent_done;
        }
    }
    SuperWindowEvent(sender, event, param);
}

WindowNumbPrintProgress::WindowNumbPrintProgress(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_numb_t>(parent, rect)
    , last_sd_percent_done(-1) {
    font = resource_font(IDR_FNT_BIG);
    SetAlignment(Align_t::Center());
    PrintAsInt();
    SetFormat("%d%%");
}

void WindowNumbPrintProgress::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        if (marlin_vars()->sd_percent_done != last_sd_percent_done) {
            last_sd_percent_done = marlin_vars()->sd_percent_done;
            SetValue(marlin_vars()->sd_percent_done);
        }
    }
    SuperWindowEvent(sender, event, param);
}
