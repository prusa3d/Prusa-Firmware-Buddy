//screen_sysinf.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_numb.hpp"

struct screen_sysinfo_data_t : public window_frame_t {
    window_text_t textMenuName;
    window_text_t textCPU_load;
    window_numb_t textCPU_load_val;

    window_text_t textExit;

public:
    screen_sysinfo_data_t();

private:
    virtual int event(window_t *sender, uint8_t event, void *param) override;
};
