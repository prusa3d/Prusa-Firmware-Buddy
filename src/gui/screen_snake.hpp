//screen_snake.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_numb.hpp"
#include "guitypes.hpp"
#include "cmath_ext.h"
#include "scratch_buffer.hpp"

using point_ui8_t = point_t<uint8_t>;
using point_i8_t = point_t<int8_t>;
const constexpr int snake_max_length = SCRATCH_BUFFER_SIZE / sizeof(point_ui8_t);

class screen_snake_data_t : public AddSuperWindow<window_frame_t> {
public:
    screen_snake_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    uint32_t last_redraw = 0;
    point_i8_t direction = { 0, -1 };
    /// circular buffer
    point_ui8_t *snake = (point_ui8_t *)scratch_buffer;
    int buffer_pos = 0;
    int snake_length = 1;
    point_ui8_t food;
    bool stop = false;
    /// avoids going in reverse & knob acceleration
    int changes = 0;

    void move_snake();
    void generate_food();
    void draw_block(const point_ui8_t point, const color_t color);
    void check_food();
    bool collision();
    void draw_food();
};
