// window_progress.hpp

#pragma once

#include "window_progress.hpp"

// to be used in ctor
// could use template with window_progress_t and window_progress_numberless_t instead
// but it would generate more code
enum class HasNumber_t {
    no,
    yes
};

class WindowPrintProgress : public AddSuperWindow<window_numberless_progress_t> {
    int8_t last_sd_percent_done;
    void update_progress(uint8_t percent, uint16_t print_speed);

public:
    WindowPrintProgress(window_t *parent, Rect16 rect);

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
