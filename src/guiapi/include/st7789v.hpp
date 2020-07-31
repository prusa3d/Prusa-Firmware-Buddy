//st7789v.hpp
//C++ wrapper over st7789v so it does not need to knoe advanced types like font or rectangle
#pragma once

#include "st7789v.h"

void st7789v_set_pixel(point_ui16_t pt, color_t clr);

void st7789v_draw_rect(rect_ui16_t rc, color_t clr);

void st7789v_fill_rect(rect_ui16_t rc, color_t clr);

bool st7789v_draw_charUnicode(point_ui16_t pt, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg);

bool st7789v_draw_text(rect_ui16_t rc, const char *str, const font_t *pf, color_t clr_bg, color_t clr_fg);

void st7789v_clip_rect(rect_ui16_t rc);
