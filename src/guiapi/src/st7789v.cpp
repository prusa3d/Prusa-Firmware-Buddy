// st7789v.cpp
#include "st7789v.hpp"
#include <functional>
#include <cmath>

extern "C" {

extern uint8_t st7789v_buff[ST7789V_COLS * 2 * 16]; //16 lines buffer

extern void st7789v_draw_char_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

} //extern "C"

static constexpr Rect16 st7789v_clip = { 0, 0, ST7789V_COLS, ST7789V_ROWS };

static bool st7789v_draw_char(point_ui16_t pt, char chr, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    const uint16_t w = pf->w; //char width
    const uint16_t h = pf->h; //char height
    // character out of font range, display solid rectangle instead
    if ((chr < pf->asc_min) || (chr > pf->asc_max)) {
        st7789v_fill_rect(Rect16(pt.x, pt.y, w, h), clr_bg);
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
bool st7789v_draw_text(Rect16 rc, const char *str, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    int x = rc.Left();
    int y = rc.Top();

    const uint16_t rc_end_x = rc.Left() + rc.Width();
    const uint16_t rc_end_y = rc.Top() + rc.Height();
    const uint16_t w = pf->w; //char width
    const uint16_t h = pf->h; //char height

    // prepare for stream processing
    char c = 0;
    while ((c = *str++) != 0) {
        if (c == '\n') {
            y += h;
            x = rc.Left();
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
void st7789v_draw_rect(Rect16 rc, color_t clr) {
    if (rc.Width() <= 0 || rc.Height() <= 0)
        return;

    point_i16_t pt0 = rc.TopLeft();
    point_i16_t pt1 = { int16_t(rc.Left() + rc.Width() - 1), rc.Top() };
    point_i16_t pt2 = { rc.Left(), int16_t(rc.Top() + rc.Height() - 1) };

    st7789v_fill_rect(Rect16(pt0, rc.Width(), 1), clr);  // top
    st7789v_fill_rect(Rect16(pt0, 1, rc.Height()), clr); // left
    st7789v_fill_rect(Rect16(pt1, 1, rc.Height()), clr); // right
    st7789v_fill_rect(Rect16(pt2, rc.Width(), 1), clr);  // bottom
}

void st7789v_fill_rect(Rect16 rc, color_t clr) {
    rc = rc.Intersection(st7789v_clip);
    if (rc.IsEmpty())
        return;
    uint16_t clr565 = color_to_565(clr);
    st7789v_fill_rect_C(rc.Left(), rc.Top(), rc.Width(), rc.Height(), clr565);
}

/// Turns the specified pixel to the specified color
void st7789v_set_pixel(point_ui16_t pt, color_t clr) {
    if (!st7789v_clip.Contain(pt))
        return;
    uint16_t clr565 = color_to_565(clr);
    st7789v_set_pixel_C(pt.x, pt.y, clr565);
}

/// Draws simple line (no antialiasing)
/// Both end points are drawn
void st7789v_draw_line(point_ui16_t pt0, point_ui16_t pt1, color_t clr) {
    const uint16_t clr565 = color_to_565(clr);
    //todo check rectangle
    int n;
    const int dx = pt1.x - pt0.x;
    const int dy = pt1.y - pt0.y;
    int cx = std::abs(dx);
    int cy = std::abs(dy);
    const int adx = cx; // absolute difference in x ( = width - 1)
    const int ady = cy; // absolute difference in y ( = height - 1)

    if ((adx == 0) || (ady == 0)) { // orthogonal line
        st7789v_fill_rect_C(std::min(pt0.x, pt1.x), std::min(pt0.y, pt1.y), adx + 1, ady + 1, clr565);
        return;
    }

    const int sx = std::signbit(dx) ? -1 : 1;
    const int sy = std::signbit(dy) ? -1 : 1;

    if (adx > ady) { // likely vertical line
        for (n = adx; n > 0; --n) {
            st7789v_set_pixel_C(pt0.x, pt0.y, clr565);
            if ((cx -= cy) <= 0) {
                pt0.y += sy;
                cx += adx;
            }
            pt0.x += sx;
        }
        return;
    }

    if (adx < ady) { // likely horizontal line
        for (n = ady; n > 0; --n) {
            st7789v_set_pixel_C(pt0.x, pt0.y, clr565);
            if ((cy -= cx) <= 0) {
                pt0.x += sx;
                cy += ady;
            }
            pt0.y += sy;
        }
        return;
    }

    //adx == ady => diagonal line
    for (n = adx; n > 0; --n) {
        st7789v_set_pixel_C(pt0.x, pt0.y, clr565);
        pt0.x += sx;
        pt0.y += sy;
    }
}

color_t st7789v_get_pixel(point_ui16_t pt) {
    if (!st7789v_clip.Contain(pt))
        return 0;
    return st7789v_get_pixel_C(pt.x, pt.y);
}

void st7789v_set_pixel_directColor(point_ui16_t pt, uint16_t noClr) {
    if (!st7789v_clip.Contain(pt))
        return;
    st7789v_set_pixel_directColor_C(pt.x, pt.y, noClr);
}

uint16_t st7789v_get_pixel_directColor(point_ui16_t pt) {
    if (!st7789v_clip.Contain(pt))
        return 0;
    return st7789v_get_pixel_directColor_C(pt.x, pt.y);
}

void st7789v_draw_icon(point_ui16_t pt, uint16_t id_res, color_t clr0, uint8_t rop) {
    FILE *pf = resource_fopen(id_res, "rb");
    st7789v_draw_png_ex(pt.x, pt.y, pf, clr0, rop);
    fclose(pf);
}

void st7789v_draw_png(point_ui16_t pt, FILE *pf) {
    st7789v_draw_png_ex(pt.x, pt.y, pf, 0, 0);
}
