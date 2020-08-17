//st7789v.hpp
//C++ wrapper over st7789v so it does not need to knoe advanced types like font or rectangle
#pragma once

#include "st7789v.h"
#include "guitypes.hpp"
#include "Rect16.h"

inline void st7789v_clear(const color_t clr) {
    const uint16_t clr565 = color_to_565(clr);
    st7789v_clear_C(clr565);
}
void st7789v_set_pixel(point_ui16_t pt, color_t clr);

void st7789v_draw_rect(Rect16 rc, color_t clr);

void st7789v_fill_rect(Rect16 rc, color_t clr);

bool st7789v_draw_charUnicode(point_ui16_t pt, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg);

bool st7789v_draw_text(Rect16 rc, const char *str, const font_t *pf, color_t clr_bg, color_t clr_fg);

void st7789v_draw_line(point_ui16_t pt0, point_ui16_t pt1, color_t clr);

color_t st7789v_get_pixel(point_ui16_t pt);

void st7789v_set_pixel_directColor(point_ui16_t pt, uint16_t noClr);

uint16_t st7789v_get_pixel_directColor(point_ui16_t pt);

void st7789v_draw_png(point_ui16_t pt, FILE *pf);

void st7789v_draw_icon(point_ui16_t pt, uint16_t id_res, color_t clr0, uint8_t rop);
