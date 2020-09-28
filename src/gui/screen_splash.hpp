#pragma once
#include "gui.hpp"

struct screen_splash_data_t : public window_frame_t {
    window_icon_t logo_prusa_mini;
    window_text_t text_progress;
    window_progress_t progress;
    window_text_t text_version;
    char text_version_buffer[16];
    window_icon_t icon_logo_buddy;
    window_icon_t icon_logo_marlin;

    window_icon_t icon_debug;

    // uint32_t last_timer;

    screen_splash_data_t();

private:
    void timer(uint32_t mseconds);
    virtual void draw() override;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
