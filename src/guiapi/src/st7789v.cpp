// st7789v.cpp
#include "st7789v.hpp"

extern "C" {

extern uint8_t st7789v_buff[ST7789V_COLS * 2 * 16]; //16 lines buffer
extern rect_ui16_t st7789v_clip;

extern void st7789v_draw_char_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
extern void st7789v_set_pixel_C(uint16_t point_x, uint16_t point_y, uint32_t clr);
extern void st7789v_fill_rect_C(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t clr);

} //extern "C"

void st7789v_clip_rect(rect_ui16_t rc) {
    st7789v_clip = rc;
}

static bool st7789v_draw_char(point_ui16_t pt, char chr, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    const uint16_t w = pf->w; //char width
    const uint16_t h = pf->h; //char height
    // character out of font range, display solid rectangle instead
    if ((chr < pf->asc_min) || (chr > pf->asc_max)) {
        st7789v_fill_rect(rect_ui16(pt.x, pt.y, w, h), clr_bg);
        return false;
    }
    // here we only have an ASCII character, its location in font can be computed easily
    uint8_t charX = (chr - pf->asc_min) % 16;
    uint8_t charY = (chr - pf->asc_min) / 16;
    return st7789v_draw_charUnicode(pt, charX, charY, pf, clr_bg, clr_fg);
}

/// Draws a single character according to selected font
/// \param charX x-index of character in font bitmap
/// \param charY y-index of character in font bitmap
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// If font is not available for the character, solid rectangle will be drawn in background color
/// \returns true if character is available in the font and was drawn
bool st7789v_draw_charUnicode(point_ui16_t pt, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    const uint16_t w = pf->w; //char width
    const uint16_t h = pf->h; //char height

    int i;
    int j;
    uint8_t *pch;    //character data pointer
    uint8_t crd = 0; //current row byte data
    uint8_t rb;      //row byte
    uint16_t *p = (uint16_t *)st7789v_buff;
    uint8_t *pc;

    const uint8_t bpr = pf->bpr;        //bytes per row
    const uint16_t bpc = bpr * h;       //bytes per char
    const uint8_t bpp = 8 * bpr / w;    //bits per pixel
    const uint8_t ppb = 8 / bpp;        //pixels per byte
    const uint8_t pms = (1 << bpp) - 1; //pixel mask

    uint32_t chr = charY * 16 + charX; // compute character index in font

    pch = (uint8_t *)(pf->pcs) + ((chr /*- pf->asc_min*/) * bpc);
    uint16_t clr565[16];
    for (i = 0; i <= pms; i++)
        clr565[i] = color_to_565(color_alpha(clr_bg, clr_fg, 255 * i / pms));
    for (j = 0; j < h; j++) {
        pc = pch + j * bpr;
        for (i = 0; i < w; i++) {
            if ((i % ppb) == 0) {
                if (pf->flg & FONT_FLG_SWAP) {
                    rb = (i / ppb) ^ 1;
                    crd = pch[rb + j * bpr];
                } else
                    crd = *(pc++);
            }
            if (pf->flg & FONT_FLG_LSBF) {
                *(p++) = clr565[crd & pms];
                crd >>= bpp;
            } else {
                *(p++) = clr565[crd >> (8 - bpp)];
                crd <<= bpp;
            }
        }
    }
    st7789v_draw_char_from_buffer(pt.x, pt.y, w, h);

    return true;
}

/// Draws a text into the specified rectangle @rc
/// If a character does not fit into the rectangle the drawing is stopped
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns true if whole text was written
bool st7789v_draw_text(rect_ui16_t rc, const char *str, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    int x = rc.x;
    int y = rc.y;

    const uint16_t rc_end_x = rc.x + rc.w;
    const uint16_t rc_end_y = rc.y + rc.h;
    const uint16_t w = pf->w; //char width
    const uint16_t h = pf->h; //char height

    // prepare for stream processing
    char c = 0;
    while ((c = *str++) != 0) {
        if (c == '\n') {
            y += h;
            x = rc.x;
            if (y + h > rc_end_y)
                return false;
            continue;
        }

        st7789v_draw_char(point_ui16(x, y), c, pf, clr_bg, clr_fg);
        x += w;
        // FIXME Shouldn't it try to break the line first?
        if (x + w > rc_end_x)
            return false;
    }
    return true;
}

/// Draws a rectangle boundary of defined color
void st7789v_draw_rect(rect_ui16_t rc, color_t clr) {
    if (rc.w <= 0 || rc.h <= 0)
        return;

    point_ui16_t pt0 = { rc.x, rc.y };
    point_ui16_t pt1 = { rc.x + rc.w - 1, rc.y };
    point_ui16_t pt2 = { rc.x, rc.y + rc.h - 1 };

    st7789v_fill_rect(rect_ui16(pt0.x, pt0.y, rc.w, 1), clr); // top
    st7789v_fill_rect(rect_ui16(pt0.x, pt0.y, 1, rc.h), clr); // left
    st7789v_fill_rect(rect_ui16(pt1.x, pt1.y, 1, rc.h), clr); // right
    st7789v_fill_rect(rect_ui16(pt2.x, pt2.y, rc.w, 1), clr); // bottom
}

void st7789v_fill_rect(rect_ui16_t rc, color_t clr) {
    rc = rect_intersect_ui16(rc, st7789v_clip);
    if (rect_empty_ui16(rc))
        return;

    st7789v_fill_rect_C(rc.x, rc.y, rc.w, rc.h, clr);
}

/// Turns the specified pixel to the specified color
void st7789v_set_pixel(point_ui16_t pt, color_t clr) {
    if (!point_in_rect_ui16(pt, st7789v_clip))
        return;
    st7789v_set_pixel_C(pt.x, pt.y, clr);
}
