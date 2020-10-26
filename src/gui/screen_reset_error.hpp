//screen_reset_perror.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"

class screen_reset_error_data_t : public window_frame_t {

public:
    screen_reset_error_data_t();

protected:
    /// starts sound and avoids repetitive starting
    void start_sound();
    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    bool sound_started;
};
