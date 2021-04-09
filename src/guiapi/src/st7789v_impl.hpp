/**
 * @file
 * @brief st7789 implementation details, do not use outside st7789 implementation
 */
#pragma once
#include "st7789v.hpp"
#include <stdint.h>

void st7789v_draw_char_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
extern uint8_t st7789v_buff[ST7789V_COLS * 2 * ST7789V_BUFF_ROWS]; //16 lines buffer
