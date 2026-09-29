// window_print_progress.cpp
#include "window_print_progress.hpp"
#include "gui.hpp"
#include <algorithm>

/*****************************************************************************/
// WindowPrintProgress
#include "marlin_client.hpp"
#include <common/marlin_vars.hpp>
WindowPrintProgress::WindowPrintProgress(window_t *parent, Rect16 rect)
    : WindowProgressBar(parent, rect, COLOR_BRAND, COLOR_GRAY)
    , last_sd_percent_done(-1) {
}

void WindowPrintProgress::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        if (marlin_vars().sd_percent_done != last_sd_percent_done) {
            set_progress_percent(marlin_vars().sd_percent_done);
            last_sd_percent_done = marlin_vars().sd_percent_done;
        }
    }
    WindowProgressBar::windowEvent(sender, event, param);
}

WindowNumbPrintProgress::WindowNumbPrintProgress(window_t *parent, Rect16 rect)
    : window_numb_t(parent, rect)
    , last_sd_percent_done(-1) {
    set_font(Font::big);
    SetAlignment(Align_t::Center());
    PrintAsInt32();
    SetFormat("%d%%");
}

void WindowNumbPrintProgress::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        if (marlin_vars().sd_percent_done != last_sd_percent_done) {
            last_sd_percent_done = marlin_vars().sd_percent_done;
            SetValue(marlin_vars().sd_percent_done);
            percent_changed = true;
        }
    }
    window_numb_t::windowEvent(sender, event, param);
}

int8_t WindowNumbPrintProgress::getPercentage() {
    return last_sd_percent_done;
}
