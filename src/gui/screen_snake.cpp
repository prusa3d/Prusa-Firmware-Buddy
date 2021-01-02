/*
 * screen_sysinf.cpp
 *
 *  Created on: 2019-09-25
 *      Author: Radek Vana
 */

#include "screen_snake.hpp"
#include "display.h"
#include "ScreenHandler.hpp"

/// this defines movement speed in miliseconds
static const constexpr int movement_delay = 100;
static const constexpr color_t color_snake = COLOR_ORANGE;
static const constexpr color_t color_food = COLOR_LIME;
static const constexpr color_t color_bg = COLOR_BLACK;
static const constexpr int block_size = 3;
static const constexpr point_ui8_t blocks = { 80, 106 };

screen_snake_data_t::screen_snake_data_t()
    : AddSuperWindow<screen_t>() {
    snake[0].x = blocks.x / 2;
    snake[0].y = blocks.y / 2;
    food.x = snake[0].x;
    food.y = snake[0].y - 1;
}

void screen_snake_data_t::draw_block(point_ui8_t point, color_t color) {
    display::FillRect(Rect16(point.x * block_size, point.y * block_size, block_size, block_size), color);
}

void screen_snake_data_t::generate_food() {
    bool ok = false;
    while (!ok) {
        food.x = RAND(blocks.x);
        food.y = RAND(blocks.y);
        ok = true;
        for (int i = 0; i < snake_length; ++i) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                ok = false;
                break;
            }
        }
    }
    draw_block(food, color_food);
}

void screen_snake_data_t::check_food(uint8_t idx) {
    if (snake[idx].x != food.x && snake[idx].y != food.y)
        return;
    if (snake_length >= snake_max_length)
        return;
    snake[snake_length] = { 255, 255 };
    ++snake_length;
}

bool screen_snake_data_t::collision(uint8_t idx) {
    for (int i = 0; i < snake_length; ++i) {
        if (snake[i].x == snake[idx].x && snake[i].y == snake[idx].y && i != idx)
            return true;
    }
    return false;
}

void screen_snake_data_t::move_snake() {
    const int tip_idx = buffer_pos;
    buffer_pos = (buffer_pos + 1) % snake_length;
    draw_block(snake[buffer_pos], color_bg);

    snake[buffer_pos].x = (snake[tip_idx].x + (int)direction.x + blocks.x) % blocks.x;
    snake[buffer_pos].y = (snake[tip_idx].y + (int)direction.y + blocks.y) % blocks.y;
    if (collision(buffer_pos)) {
        stop = true;
        return;
    }
    draw_block(snake[buffer_pos], color_snake);
}

void screen_snake_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        Screens::Access()->Close();
    }

    if (stop)
        return;

    uint32_t now = HAL_GetTick();
    if (now - last_redraw > movement_delay) {
        last_redraw = now;
        move_snake();
    }

    const uint8_t old_x = direction.x;
    if (event == GUI_event_t::ENC_UP) {
        direction.x = -direction.y;
        direction.y = old_x;
    }

    if (event == GUI_event_t::ENC_DN) {
        direction.x = direction.y;
        direction.y = -old_x;
    }
}
