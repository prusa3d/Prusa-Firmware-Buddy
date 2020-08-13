//helper.h
#pragma once

#include "window.hpp"
#include "guitypes.hpp"
#include "../../lang/string_view_utf8.hpp"

#define RENDER_FLG_ALIGN           0x00ff       // alignment mask (ALIGN_xxx)
#define RENDER_FLG_ROPFN           0x0f00       // raster operation function mask (ROPFN_xxx << 8)
#define RENDER_FLG_WORDB           0x1000       // multiline text
#define RENDER_FLG(a, r)           (a | r << 8) // render flag macro (ALIGN and ROPFN)
#define TEXT_ROLL_DELAY_MS         20           //todo i think system cannot shoot events this fast
#define TEXT_ROLL_INITIAL_DELAY_MS 1000
#define TXTROLL_SETUP_INIT         0
#define TXTROLL_SETUP_DONE         1
#define TXTROLL_SETUP_IDLE         2

typedef enum {
    ROLL_SETUP = 0,
    ROLL_GO,
    ROLL_STOP,
    ROLL_RESTART,
} TXTROLL_PHASE_t;

struct txtroll_t {
    rect_ui16_t rect;
    uint16_t progress;
    uint16_t count;
    uint8_t phase;
    uint8_t setup;
    uint8_t px_cd;
};

extern bool render_text(rect_ui16_t rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg);
/// FIXME add \param flags documentation
extern void render_text_align(rect_ui16_t rc, string_view_utf8 text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, uint16_t flags);
extern void render_icon_align(rect_ui16_t rc, uint16_t id_res, color_t clr0, uint16_t flags);
extern void render_unswapable_icon_align(rect_ui16_t rc, uint16_t id_res, color_t clr0, uint16_t flags);

extern void roll_text_phasing(window_t *pWin, font_t *font, txtroll_t *roll);

extern void roll_init(rect_ui16_t rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint8_t alignment, txtroll_t *roll);
extern void render_roll_text_align(rect_ui16_t rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint8_t alignment, color_t clr_back, color_t clr_text, const txtroll_t *roll);
extern point_ui16_t font_meas_text(const font_t *pf, string_view_utf8 *str, uint16_t *numOfUTF8Chars);
//extern int font_line_chars(const font_t *pf, string_view_utf8 str, uint16_t line_width);
extern uint16_t text_rolls_meas(rect_ui16_t rc, string_view_utf8 text, const font_t *pf);
extern rect_ui16_t roll_text_rect_meas(rect_ui16_t rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint16_t flags);
extern void fill_between_rectangles(const rect_ui16_t *r_out, const rect_ui16_t *r_in, color_t color);
