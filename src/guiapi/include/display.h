//display.h
#pragma once

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

#include "guitypes.h"

typedef struct _display_t display_t;

typedef void(display_init_t)(void);
typedef void(display_done_t)(void);
typedef void(display_clear_t)(color_t clr);
typedef void(display_set_pixel_t)(point_ui16_t pt, color_t clr);
typedef void(display_draw_line_t)(point_ui16_t pt0, point_ui16_t pt1, color_t clr);
typedef void(display_draw_rect_t)(rect_ui16_t rc, color_t clr);
typedef void(display_fill_rect_t)(rect_ui16_t rc, color_t clr);
typedef bool(display_draw_char_t)(point_ui16_t pt, char chr, const font_t *pf, color_t clr_bg, color_t clr_fg);
typedef bool(display_draw_text_t)(rect_ui16_t rc, const char *str, const font_t *pf, color_t clr_bg, color_t clr_fg);
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

template <uint16_t W, uint16_t H, display_init_t *INIT, display_done_t *DONE, display_clear_t *CLEAR, display_set_pixel_t *SET_PIXEL, display_draw_line_t *DRAW_LINE, display_draw_rect_t *DRAW_RECT, display_fill_rect_t *FIL_RECT, display_draw_char_t *DRAW_CHAR, display_draw_text_t *DRAW_TEXT, display_draw_icon_t *DRAW_ICON, display_draw_png_t *DRAW_PNG>
class Display {
public:
    constexpr static uint16_t GetW() { return W; }
    constexpr static uint16_t GetH() { return H; }
    constexpr static void Init() { INIT(); }
    constexpr static void Done() { DONE(); }
    constexpr static void Clear(color_t clr) { CLEAR(clr); }
    constexpr static void SetPixel(point_ui16_t pt, color_t clr) { SET_PIXEL(pt, clr); }
    constexpr static void DrawLine(point_ui16_t pt0, point_ui16_t pt1, color_t clr) { DRAW_LINE(pt0, pt1, clr); }
    constexpr static void DrawRect(rect_ui16_t rc, color_t clr) { DRAW_RECT(rc, clr); }
    constexpr static void FillRect(rect_ui16_t rc, color_t clr) { FIL_RECT(rc, clr); }
    constexpr static bool DrawChar(point_ui16_t pt, char chr, const font_t *pf, color_t clr_bg, color_t clr_fg) { return DRAW_CHAR(pt, chr, pf, clr_bg, clr_fg); }
    constexpr static bool DrawText(rect_ui16_t rc, const char *str, const font_t *pf, color_t clr_bg, color_t clr_fg) { return DRAW_TEXT(rc, str, pf, clr_bg, clr_fg); }
    constexpr static void DrawIcon(point_ui16_t pt, uint16_t id_res, color_t clr0, uint8_t rop) { DRAW_ICON(pt, id_res, clr0, rop); }
    constexpr static void DrawPng(point_ui16_t pt, FILE *pf) { DRAW_PNG(pt, pf); }
};

extern "C" {
#endif //__cplusplus

extern const display_t *const display;

#ifdef __cplusplus
}
#endif //__cplusplus
/*

*/
