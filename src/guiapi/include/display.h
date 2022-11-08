//display.h
#pragma once

#include <inttypes.h>
#include <algorithm>
#include "guiconfig.h"
#include "guitypes.hpp"
#include "display_helper.h"
#include "Rect16.h"
#include "display_ex.hpp"
#include "resource.h"

typedef uint16_t(display_size_t)(void);
typedef void(display_init_t)(void);
typedef void(display_init_t)(void);
typedef void(display_done_t)(void);
typedef void(display_clear_t)(color_t clr);
typedef void(display_set_pixel_t)(point_ui16_t pt, color_t clr);
typedef uint8_t *(display_get_block_t)(point_ui16_t start, point_ui16_t end);
typedef void(display_draw_line_t)(point_ui16_t pt0, point_ui16_t pt1, color_t clr);
typedef void(display_draw_rect_t)(Rect16 rc, color_t clr);
typedef void(display_fill_rect_t)(Rect16 rc, color_t clr);

/// @param charX x-coordinate of character (glyph) in font bitmap (remember, fonts are bitmaps 16 chars wide and arbitrary lines of chars tall)
/// @param charY y-coordinate of character (glyph) in font bitmap
typedef bool(display_draw_char_t)(point_ui16_t pt, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg);
typedef size_ui16_t(display_draw_text_t)(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg);
typedef uint32_t(display_buffer_pixel_size_t)();
typedef void(display_store_char_in_buffer_t)(uint16_t char_cnt, uint16_t curr_char_idx, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg);
typedef void(display_draw_from_buffer_t)(point_ui16_t pt, uint16_t w, uint16_t h);
typedef void(display_draw_png_t)(point_ui16_t pt, const png::Resource &png, color_t clr_back, ropfn rop);
typedef void(display_read_madctl_t)(uint8_t *pdata, uint8_t size);

template <
#ifndef USE_MOCK_DISPLAY // mock display has dynamical size
    uint16_t W, uint16_t H
#else  // USE_MOCK_DISPLAY
    display_size_t *COLS, display_size_t *ROWS
#endif // USE_MOCK_DISPLAY
    ,
    display_init_t *INIT, display_done_t *DONE, display_clear_t *CLEAR, display_set_pixel_t *SET_PIXEL, display_get_block_t *GET_BLOCK,
    display_draw_line_t *DRAW_LINE, display_draw_rect_t *DRAW_RECT, display_fill_rect_t *FIL_RECT, display_draw_char_t *DRAW_CHAR,
    display_draw_text_t *DRAW_TEXT, display_buffer_pixel_size_t *BUFFER_PIXEL_SIZE, display_store_char_in_buffer_t *STORE_CHAR_IN_BUFFER,
    display_draw_from_buffer_t *DRAW_FROM_BUFFER, display_draw_png_t *DRAW_PNG,
    display_read_madctl_t *READ_MADCLT>
class Display {
    // sorted raw array of known utf8 character indices
public:
    /// Get width or height  of display
#ifndef USE_MOCK_DISPLAY // mock display has dynamical size
    constexpr static uint16_t GetW() { return W; }
    constexpr static uint16_t GetH() { return H; }
#else  // USE_MOCK_DISPLAY
    constexpr static uint16_t GetW() { return COLS(); }
    constexpr static uint16_t GetH() { return ROWS(); }
#endif // USE_MOCK_DISPLAY

    constexpr static void Init() { INIT(); }
    constexpr static void Done() { DONE(); }
    constexpr static void Clear(color_t clr) { CLEAR(clr); }
    constexpr static void SetPixel(point_ui16_t pt, color_t clr) { SET_PIXEL(pt, clr); }
    constexpr static uint8_t *GetBlock(point_ui16_t start, point_ui16_t end) { return GET_BLOCK(start, end); }
    constexpr static void DrawLine(point_ui16_t pt0, point_ui16_t pt1, color_t clr) { DRAW_LINE(pt0, pt1, clr); }
    constexpr static void DrawLine(point_i16_t pt0, point_i16_t pt1, color_t clr) {
        uint16_t l = std::max(int16_t(0), pt0.x);
        uint16_t t = std::max(int16_t(0), pt0.y);
        uint16_t r = std::max(int16_t(0), pt1.x);
        uint16_t b = std::max(int16_t(0), pt1.y);
        DrawLine(point_ui16(l, t), point_ui16(r, b), clr);
    }
    constexpr static void DrawRect(Rect16 rc, color_t clr) { DRAW_RECT(rc, clr); }
    constexpr static void FillRect(Rect16 rc, color_t clr) { FIL_RECT(rc, clr); }
    constexpr static bool DrawChar(point_ui16_t pt, unichar c, const font_t *pf, color_t clr_bg, color_t clr_fg) {
        uint8_t charX = 0, charY = 0;
        get_char_position_in_font(c, pf, &charX, &charY);
        return DRAW_CHAR(pt, charX, charY, pf, clr_bg, clr_fg);
    }
    /// Draws text on the display
    /// \param rc rectangle where text will be placed
    static size_ui16_t DrawText(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg) { return DRAW_TEXT(rc, str, pf, clr_bg, clr_fg); }
    constexpr static uint32_t BufferPixelSize() { return BUFFER_PIXEL_SIZE(); }
    constexpr static void StoreCharInBuffer(uint16_t char_cnt, uint16_t curr_char_idx, unichar c, const font_t *pf, color_t clr_bg, color_t clr_fg) {
        uint8_t charX = 0, charY = 0;
        get_char_position_in_font(c, pf, &charX, &charY);
        STORE_CHAR_IN_BUFFER(char_cnt, curr_char_idx, charX, charY, pf, clr_bg, clr_fg);
    }
    constexpr static void DrawFromBuffer(point_ui16_t pt, uint16_t w, uint16_t h) { DRAW_FROM_BUFFER(pt, w, h); }
    constexpr static void DrawPng(point_ui16_t pt, const png::Resource &png, color_t clr_back = 0, ropfn rop = ropfn()) { DRAW_PNG(pt, png, clr_back, rop); }
    constexpr static void ReadMADCTL(uint8_t *pdata, uint8_t size) { READ_MADCLT(pdata, size); }
};

#ifdef USE_ST7789
    #include "st7789v.hpp"
using display = Display<ST7789V_COLS, ST7789V_ROWS,
    st7789v_init,
    st7789v_done,
    display_ex_clear,
    display_ex_set_pixel,
    display_ex_get_block,
    display_ex_draw_line,
    display_ex_draw_rect,
    display_ex_fill_rect,
    display_ex_draw_char,
    render_text_singleline,
    display_ex_buffer_pixel_size,
    display_ex_store_char_in_buffer,
    display_ex_draw_from_buffer,
    display_ex_draw_png,
    st7789v_cmd_madctlrd>;
#endif

#ifdef USE_MOCK_DISPLAY
    #include "mock_display.hpp"
using display = Display<MockDisplay::Cols, MockDisplay::Rows,
    MockDisplay::init,
    MockDisplay::done,
    display_ex_clear,
    display_ex_set_pixel,
    display_ex_get_block,
    display_ex_draw_line,
    display_ex_draw_rect,
    display_ex_fill_rect,
    display_ex_draw_char,
    render_text_singleline,
    display_ex_buffer_pixel_size,
    display_ex_store_char_in_buffer,
    display_ex_draw_from_buffer,
    display_ex_draw_png,
    MockDisplay::ReadMadctl>;
#endif
