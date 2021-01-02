// screen_snake.cpp

#include "screen_snake.hpp"
#include "display.h"
#include "ScreenHandler.hpp"

/// this defines movement speed in miliseconds
static const constexpr int movement_delay = 100;
static const constexpr color_t color_snake = COLOR_ORANGE;
static const constexpr color_t color_food = COLOR_LIME;
static const constexpr color_t color_bg = COLOR_BLACK;
static const constexpr color_t color_dead = COLOR_RED;
static const constexpr int block_size = 5;
static const constexpr point_ui8_t blocks = { 240 / block_size, 320 / block_size };

screen_snake_data_t::screen_snake_data_t()
    : AddSuperWindow<screen_t>() {
    flags.timeout_close = is_closed_on_timeout_t::no;
    snake[0].x = blocks.x / 2;
    snake[0].y = blocks.y / 2;
    food.x = snake[0].x;
    food.y = snake[0].y - 1;
}

void screen_snake_data_t::draw_block(const point_ui8_t point, const color_t color) {
    display::FillRect(Rect16(point.x * block_size + 1, point.y * block_size + 1, block_size - 2, block_size - 2), color);
}

void screen_snake_data_t::draw_food() {
    draw_block(food, color_food);
}

void screen_snake_data_t::generate_food() {
    bool ok;
    do {
        const uint32_t time = HAL_GetTick();
        food.x = time % blocks.x;
        food.y = time % blocks.y;
        ok = true;
        for (int i = 0; i < snake_length; ++i) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                ok = false;
                break;
            }
        }
    } while (!ok);
}

void screen_snake_data_t::check_food() {
    if (snake[buffer_pos].x == food.x && snake[buffer_pos].y == food.y) {
        if (snake_length < snake_max_length) {
            // snake[snake_length] = { 255, 255 };
            snake_length++;
        }
        generate_food();
    }
}

bool screen_snake_data_t::collision() {
    for (int i = 0; i < snake_length; ++i) {
        if (snake[i].x == snake[buffer_pos].x && snake[i].y == snake[buffer_pos].y && i != buffer_pos)
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
    if (collision()) {
        stop = true;
        // draw_block(snake[buffer_pos], color_dead);
        return;
    }
    draw_block(snake[buffer_pos], color_snake);
    check_food();
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
        draw_food();
        changes = 0;
    }

    const uint8_t old_x = direction.x;
    if (event == GUI_event_t::ENC_UP && changes <= 0) {
        direction.x = -direction.y;
        direction.y = old_x;
        ++changes;
    }

    if (event == GUI_event_t::ENC_DN && changes >= 0) {
        direction.x = direction.y;
        direction.y = -old_x;
        --changes;
    }
}
