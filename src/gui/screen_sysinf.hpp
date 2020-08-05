//screen_sysinf.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_numb.hpp"

// Use this #define to hide the static display of current NTP time - only for debugging
// Clean solution will come later
#define DEBUG_NTP

#ifdef DEBUG_NTP
    #include "../lang/format_print_will_end.hpp"
    #include "wui_api.h"
#endif

struct screen_sysinfo_data_t : public window_frame_t {
    window_text_t textMenuName;
    window_text_t textCPU_load;
    window_numb_t textCPU_load_val;
#ifdef DEBUG_NTP
    window_text_t textDateTime;
#endif
    window_text_t textExit;

public:
    screen_sysinfo_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};
