//screen_temperror.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"

class screen_temperror_data_t : public window_frame_t {
    window_text_t text;
    window_text_t exit;

public:
    screen_temperror_data_t();

protected:
    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
