//display.h
#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <inttypes.h>
#include <stdio.h>

#include "guitypes.h"

typedef struct _display_t display_t;

typedef void(display_init_t)(void);
typedef void(display_done_t)(void);
typedef void(display_clear_t)(color_t clr);
typedef void(display_set_pixel_t)(point_ui16_t pt, color_t clr);
typedef void(display_draw_line_t)(point_ui16_t pt, point_ui16_t pt1, color_t clr);
typedef void(display_draw_rect_t)(rect_ui16_t rc, color_t clr);
typedef void(display_fill_rect_t)(rect_ui16_t rc, color_t clr);
typedef void(display_draw_char_t)(point_ui16_t pt, char chr, font_t *pf, color_t clr0, color_t clr1);
typedef void(display_draw_text_t)(rect_ui16_t rc, const char *str, font_t *pf, color_t clr0, color_t clr1);
typedef void(display_draw_icon_t)(point_ui16_t pt, uint16_t id_res, color_t clr0, uint8_t rop);
typedef void(display_draw_png_t)(point_ui16_t pt, FILE *pf);

typedef struct _display_t {
    uint16_t w;
    uint16_t h;
    display_init_t *init;
    display_done_t *done;
    display_clear_t *clear;
    display_set_pixel_t *set_pixel;
    display_draw_line_t *draw_line;
    display_draw_rect_t *draw_rect;
    display_fill_rect_t *fill_rect;
    display_draw_char_t *draw_char;
    display_draw_text_t *draw_text;
    display_draw_icon_t *draw_icon;
    display_draw_png_t *draw_png;
} display_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const display_t *const display;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DISPLAY_H
