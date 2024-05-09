#pragma once

#include "gui.hpp"
#include "screen.hpp"
#include "window_colored_rect.hpp"

class ScreenTouchPlayground : public screen_t {

public:
    ScreenTouchPlayground();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    window_text_t text;
    std::array<char, 64> text_content;
    window_colored_rect touch_rect;
};
