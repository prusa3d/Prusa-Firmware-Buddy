//screen_sysinf.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_numb.hpp"
#include "screen.hpp"

struct screen_snake_data_t : public AddSuperWindow<screen_t> {
public:
    screen_snake_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    virtual void unconditionalDraw() override;

private:
    uint32_t last_redraw = 0;
    int8_t direction[2] = { 0, -1 };
};
