//screen_snake.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_numb.hpp"
#include "screen.hpp"
#include "guitypes.hpp"
#include "cmath_ext.h"

using point_ui8_t = point_t<uint8_t>;
using point_i8_t = point_t<int8_t>;
const constexpr int snake_max_length = 100;

struct screen_snake_data_t : public AddSuperWindow<screen_t> {
public:
    screen_snake_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    uint32_t last_redraw = 0;
    point_i8_t direction = { 0, -1 };
    /// circular buffer
    point_ui8_t snake[snake_max_length];
    int buffer_pos = 0;
    int snake_length = 2;
    point_ui8_t food = { 0, 0 };
    bool stop = false;

    void move_snake();
    void generate_food();
    void draw_block(point_ui8_t point, color_t color);
    void check_food(uint8_t idx);
    bool collision(uint8_t idx);
};
