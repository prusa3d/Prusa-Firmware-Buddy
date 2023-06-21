/**
 * @file
 * @brief ili9488 implementation details, do not use outside ili9488 implementation
 */
#pragma once
#include "ili9488.hpp"
#include <stdint.h>

void ili9488_draw_char_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
extern uint8_t ili9488_buff[ILI9488_COLS * 3 * ILI9488_BUFF_ROWS]; // 16 lines buffer
