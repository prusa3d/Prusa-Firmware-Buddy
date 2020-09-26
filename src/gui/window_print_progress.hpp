// window_progress.hpp

#pragma once

#include "window_progress.hpp"

class WindowPrintProgress : public AddSuperWindow<window_progress_t> {
    int8_t last_sd_percent_done;
    void update_progress(uint8_t percent, uint16_t print_speed);

public:
    WindowPrintProgress(window_t *parent, point_i16_t position);

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
