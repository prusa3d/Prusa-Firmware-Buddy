// window_progress.hpp

#pragma once

#include "window_progress.hpp"

// to be used in ctor

class WindowPrintProgress : public AddSuperWindow<window_numberless_progress_t> {
    int8_t last_sd_percent_done;

public:
    WindowPrintProgress(window_t *parent, Rect16 rect);

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class WindowNumbPrintProgress : public AddSuperWindow<window_numb_t> {
    int8_t last_sd_percent_done;

public:
    WindowNumbPrintProgress(window_t *parent, Rect16 rect);

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
