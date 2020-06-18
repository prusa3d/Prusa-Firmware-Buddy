//gui.c

#include "guitypes.h"
#include <algorithm>

//intersection of positive intervals p0-p1 and p2-p3, result is stored in p4-p5
//condition (not checked): (p1 >= p0) && (p3 >= p2)
void interval_intersect_ui16(uint16_t *p) {
    p[4] = std::max(p[0], p[2]);
    p[5] = std::min(p[1], p[3]);

    if (p[4] < p[5])
        return;
    p[4] = p[5] = 0;
}

rect_ui16_t rect_intersect_ui16(rect_ui16_t rc1, rect_ui16_t rc2) {
    if (rc1.w == 0 || rc1.h == 0 || rc2.w == 0 || rc2.h == 0) {
        const rect_ui16_t rc_ret = { 0, 0, 0, 0 };
        return rc_ret;
    }

    uint16_t x[6] = { rc1.x, uint16_t(rc1.x + rc1.w), rc2.x, uint16_t(rc2.x + rc2.w), 0, 0 };
    uint16_t y[6] = { rc1.y, uint16_t(rc1.y + rc1.h), rc2.y, uint16_t(rc2.y + rc2.h), 0, 0 };
    interval_intersect_ui16(x);
    interval_intersect_ui16(y);
    const rect_ui16_t rc_ret = { x[4], y[4], uint16_t(x[5] - x[4]), uint16_t(y[5] - y[4]) };
    return rc_ret;
}

rect_ui16_t rect_ui16_add_padding_ui8(rect_ui16_t rc, padding_ui8_t pad) {
    const rect_ui16_t rect = {
        uint16_t(std::max(0, int(rc.x - pad.left))),
        uint16_t(std::max(0, int(rc.y - pad.top))),
        uint16_t(rc.w + pad.left + pad.right),
        uint16_t(rc.h + pad.top + pad.bottom)
    };
    return rect;
}

rect_ui16_t rect_ui16_sub_padding_ui8(rect_ui16_t rc, padding_ui8_t pad) {
    const rect_ui16_t rect = {
        uint16_t(rc.x + pad.left),
        uint16_t(rc.y + pad.top),
        uint16_t(std::max(0, int(rc.w - pad.left - pad.right))),
        uint16_t(std::max(0, int(rc.h - pad.top - pad.bottom)))
    };
    return rect;
}

rect_ui16_t rect_align_ui16(rect_ui16_t rc, rect_ui16_t rc1, uint8_t align) {
    rect_ui16_t rect = rc1;
    switch (align & ALIGN_HMASK) {
    case ALIGN_LEFT:
        rect.x = rc.x;
        break;
    case ALIGN_RIGHT:
        rect.x = ((rc.x + rc.w) > rc1.w) ? ((rc.x + rc.w) - rc1.w) : 0;
        break;
    case ALIGN_HCENTER:
        if (rc.w >= rc1.w)
            rect.x = rc.x + (rc.w - rc1.w) / 2;
        else
            rect.x = std::max(0, rc.x - (rc1.w - rc.w) / 2);
        break;
    }

    switch (align & ALIGN_VMASK) {
    case ALIGN_TOP:
        rect.y = rc.y;
        break;
    case ALIGN_BOTTOM:
        rect.y = ((rc.y + rc.h) > rc1.h) ? ((rc.y + rc.h) - rc1.h) : 0;
        rect.y = std::max(0, (rc.y + rc.h) - rc1.h);
        break;
    case ALIGN_VCENTER:
        if (rc.h >= rc1.h)
            rect.y = rc.y + ((rc.h - rc1.h) / 2);
        else
            rect.y = (rc.y > ((rc1.h - rc.h) / 2)) ? rc.y - ((rc1.h - rc.h) / 2) : 0;
        break;
    }
    return rect;
}

point_ui16_t font_meas_text(const font_t *pf, const char *str) {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    const int8_t char_w = pf->w;
    const int8_t char_h = pf->h;
    size_t len = strlen(str);
    while (len--) {
        const char c = *(str++);
        if (c == '\n') {
            if (x + char_w > w)
                w = x + char_w;
            y += char_h;
            x = 0;
        } else
            x += char_w;
        h = y + char_h;
    }
    return point_ui16((uint16_t)std::max(x, w), (uint16_t)h);
}

int font_line_chars(const font_t *pf, const char *str, uint16_t line_width) {
    int w = 0;
    const int char_w = pf->w;
    size_t len = strlen(str);
    size_t n = 0;
    // This is generally about finding the closest '\n' character within the current line to be drawn.
    // Line is limited by pixel dimension, all characters have the same fixed pixel size
    // Such character may not be found, so n becomes > len
    while ((w + char_w) <= line_width) {
        const char c = str[n++];
        if (c == '\n')
            break;
        w += char_w;
    }

    // if the line width is >= than characters to be printed, skip further search
    // and just return len - i.e. the whole string is to be printed at once.
    if (n >= len)
        return len; // must return here to prevent touching memory beyond str in the next while cycle

    while ((n > 0) && ((str[n] != ' ') && (str[n] != '\n'))) {
        n--;
    }

    if (n == 0)
        n = line_width / char_w;
    return std::min(n, len);
}

uint16_t text_rolls_meas(rect_ui16_t rc, const char *text, const font_t *pf) {

    uint16_t meas_x = 0, len = strlen(text);
    if (len * pf->w > rc.w)
        meas_x = len - rc.w / pf->w;
    return meas_x;
}

rect_ui16_t roll_text_rect_meas(rect_ui16_t rc, const char *text, const font_t *font, padding_ui8_t padding, uint16_t flags) {

    rect_ui16_t rc_pad = rect_ui16_sub_padding_ui8(rc, padding);
    point_ui16_t wh_txt = font_meas_text(font, text);
    rect_ui16_t rc_txt = { 0, 0, 0, 0 };
    if (wh_txt.x && wh_txt.y) {
        rc_txt = rect_align_ui16(rc_pad, rect_ui16(0, 0, wh_txt.x, wh_txt.y), flags & ALIGN_MASK);
        rc_txt = rect_intersect_ui16(rc_pad, rc_txt);
    }
    return rc_txt;
}

/// TODO refactor to icon_size
point_ui16_t icon_meas(const uint8_t *pi) {
    point_ui16_t wh = { 0, 0 };
    if (memcmp(pi, "\x89PNG", 4) == 0) {
        wh.x = swap_ui16(*((uint16_t *)(pi + 18)));
        wh.y = swap_ui16(*((uint16_t *)(pi + 22)));
    }
    return wh;
}

size_ui16_t icon_size(const uint8_t *pi) {
    const point_ui16_t p = icon_meas(pi);
    const size_ui16_t size = { p.x, p.y };
    return size;
}

extern const resource_entry_t resource_table[];
extern const uint16_t resource_table_size;
extern const uint16_t resource_count;

const uint8_t *resource_ptr(uint16_t id) {
    if (id < resource_count)
        return resource_table[id].ptr;
    return 0;
}

uint16_t resource_size(uint16_t id) {
    if (id < resource_count)
        return resource_table[id].size;
    return 0;
}

FILE *resource_fopen(uint16_t id, const char *opentype) {
    if (id < resource_count)
        return fmemopen((uint8_t *)resource_table[id].ptr, resource_table[id].size, opentype);
    return 0;
}

font_t *resource_font(uint16_t id) {
    if (id < resource_count)
        return (font_t *)resource_table[id].ptr;
    return 0;
}
