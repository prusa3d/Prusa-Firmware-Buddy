//display.h
#pragma once

#include <inttypes.h>
#include "guiconfig.h"
#include "guitypes.hpp"
#include "display_helper.h"
#include <algorithm>
#include "Rect16.h"
#include "display_ex.hpp"

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
typedef size_ui16_t(display_draw_text_t)(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg, uint16_t flags);
typedef void(display_draw_icon_t)(point_ui16_t pt, uint16_t id_res, color_t clr0, uint8_t rop);
typedef void(display_draw_png_t)(point_ui16_t pt, FILE *pf);

// just to test the FW with fonts - will be refactored
struct FCIndex {
    uint16_t unc; /// utf8 character value (stripped of prefixes)
    uint8_t charX, charY;
};

static constexpr const FCIndex fontCharIndices[] =
#include "fnt-indices.ipp"
    static constexpr const uint32_t fontCharIndicesNumItems = sizeof(fontCharIndices) / sizeof(FCIndex);

template <uint16_t W, uint16_t H, display_init_t *INIT, display_done_t *DONE, display_clear_t *CLEAR, display_set_pixel_t *SET_PIXEL, display_get_block_t *GET_BLOCK, display_draw_line_t *DRAW_LINE, display_draw_rect_t *DRAW_RECT, display_fill_rect_t *FIL_RECT, display_draw_char_t *DRAW_CHAR, display_draw_text_t *DRAW_TEXT, display_draw_icon_t *DRAW_ICON, display_draw_png_t *DRAW_PNG>
class Display {
    // sorted raw array of known utf8 character indices
public:
    /// Get width of display
    constexpr static uint16_t GetW() { return W; }
    /// Get height of display
    constexpr static uint16_t GetH() { return H; }
    constexpr static void Init() { INIT(); }
    constexpr static void Done() { DONE(); }
    constexpr static void Clear(color_t clr) { CLEAR(clr); }
    constexpr static void SetPixel(point_ui16_t pt, color_t clr) { SET_PIXEL(pt, clr); }
    constexpr static uint8_t *GetBlock(point_ui16_t start, point_ui16_t end) { return GET_BLOCK(start, end); }
    constexpr static void DrawLine(point_ui16_t pt0, point_ui16_t pt1, color_t clr) { DRAW_LINE(pt0, pt1, clr); }
    constexpr static void DrawRect(Rect16 rc, color_t clr) { DRAW_RECT(rc, clr); }
    constexpr static void FillRect(Rect16 rc, color_t clr) { FIL_RECT(rc, clr); }
    constexpr static bool DrawChar(point_ui16_t pt, unichar c, const font_t *pf, color_t clr_bg, color_t clr_fg) {
        static_assert(sizeof(FCIndex) == 4, "font char indices size mismatch");
        // convert unichar into font index - all fonts have the same layout, thus this can be computed here
        // ... and also because doing it in C++ is much easier than in plain C
        uint8_t charX = 15, charY = 1;

        if (c < pf->asc_min) { // this really happens with non-utf8 characters on filesystems
            c = '?';           // substitute with a '?' or any other suitable character, which is in the range of the fonts
        }
        // here is intentionally no else
        if (c < 128) {
            // normal ASCII character
            charX = (c - pf->asc_min) % 16;
            charY = (c - pf->asc_min) / 16;
        } else {
            // extended utf8 character - must search in the fontCharIndices map
            const FCIndex *i = std::lower_bound(fontCharIndices, fontCharIndices + fontCharIndicesNumItems, c, [](const FCIndex &i, unichar c) {
                return i.unc < c;
            });
            if (i == fontCharIndices + fontCharIndicesNumItems || i->unc != c) {
                // character not found
                charX = 15; // put '?' as a replacement
                charY = 1;
            } else {
                charX = i->charX;
                charY = i->charY;
            }
        }
        return DRAW_CHAR(pt, charX, charY, pf, clr_bg, clr_fg);
    }
    /// Draws text on the display
    /// \param rc rectangle where text will be placed
    /// \param flags if RENDER_FLG_WORDB is set, the text is wrapped to fit the rectangle,
    /// otherwise, the first line (until \0 or \n) will be drawn only.
    static size_ui16_t DrawText(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg, uint16_t flags = 0) { return DRAW_TEXT(rc, str, pf, clr_bg, clr_fg, flags); }
    constexpr static void DrawIcon(point_ui16_t pt, uint16_t id_res, color_t clr0, uint8_t rop) { DRAW_ICON(pt, id_res, clr0, rop); }
    constexpr static void DrawPng(point_ui16_t pt, FILE *pf) { DRAW_PNG(pt, pf); }
};

#ifdef USE_ST7789
    #include "st7789v.h"
using display = Display<ST7789V_COLS, ST7789V_ROWS,
    st7789v_init,
    st7789v_done,
    display_ex_clear,
    display_ex_set_pixel,
    display_ex_get_block,
    display_ex_draw_line,
    display_ex_draw_rect,
    display_ex_fill_rect,
    display_ex_draw_charUnicode,
    render_text,
    display_ex_draw_icon,
    display_ex_draw_png>;
#endif

#ifdef USE_MOCK_DISPLAY
    #include "mock_display.hpp"
using display = Display<MockDisplay::Cols(), MockDisplay::Rows(),
    MockDisplay::init,
    MockDisplay::done,
    MockDisplay::clear,
    MockDisplay::set_pixel,
    MockDisplay::get_block,
    MockDisplay::draw_line,
    MockDisplay::draw_rect,
    MockDisplay::fill_rect,
    MockDisplay::draw_charUnicode,
    render_text,
    MockDisplay::draw_icon,
    MockDisplay::draw_png>;
#endif
