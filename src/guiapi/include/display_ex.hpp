//display_ex.hpp
//C++ wrappers over low level display api (like) st7789v so it does not need to know advanced types like font or rectangle
#pragma once

#include "guitypes.hpp"
#include "Rect16.h"
#include "raster_opfn.hpp"

void display_ex_clear(const color_t clr);

void display_ex_set_pixel(point_ui16_t pt, color_t clr);

void display_ex_draw_rect(Rect16 rc, color_t clr);

void display_ex_fill_rect(Rect16 rc, color_t clr);

bool display_ex_draw_charUnicode(point_ui16_t pt, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg);

bool display_ex_draw_text(Rect16 rc, const char *str, const font_t *pf, color_t clr_bg, color_t clr_fg);

void display_ex_draw_line(point_ui16_t pt0, point_ui16_t pt1, color_t clr);

color_t display_ex_get_pixel(point_ui16_t pt);

uint8_t *display_ex_get_block(point_ui16_t start, point_ui16_t end);

void display_ex_set_pixel_displayNativeColor(point_ui16_t pt, uint16_t noClr);

uint16_t display_ex_get_pixel_displayNativeColor(point_ui16_t pt);

void display_ex_draw_png(point_ui16_t pt, FILE *pf);

void display_ex_draw_icon(point_ui16_t pt, uint16_t id_res, color_t clr0, ropfn rop);
