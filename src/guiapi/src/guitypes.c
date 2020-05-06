//gui.c

#include "guitypes.h"
#include "cmath_ext.h"

//intersection of positive intervals p0-p1 and p2-p3, result is stored in p4-p5
//condition (not checked): (p1 >= p0) && (p3 >= p2)
void interval_intersect_ui16(uint16_t *p) {
    p[4] = MAX(p[0], p[2]);
    p[5] = MIN(p[1], p[3]);

    if (p[4] < p[5])
        return;
    p[4] = p[5] = 0;
}

rect_ui16_t rect_intersect_ui16(rect_ui16_t rc, rect_ui16_t rc1) {
    uint16_t x[6] = { rc.x, rc.x + rc.w, rc1.x, rc1.x + rc1.w, 0, 0 };
    uint16_t y[6] = { rc.y, rc.y + rc.h, rc1.y, rc1.y + rc1.h, 0, 0 };
    interval_intersect_ui16(x);
    interval_intersect_ui16(y);
    rect_ui16_t rc2 = { x[4], y[4], x[5] - x[4], y[5] - y[4] };
    return rc2;
}

rect_ui16_t rect_ui16_add_padding_ui8(rect_ui16_t rc, padding_ui8_t pad) {
    rect_ui16_t rect = { 0, 0, rc.w + pad.left + pad.right, rc.h + pad.top + pad.bottom };
    if (rc.x > pad.left)
        rect.x = rc.x - pad.left;
    if (rc.y > pad.top)
        rect.y = rc.y - pad.top;
    return rect;
}

rect_ui16_t rect_ui16_sub_padding_ui8(rect_ui16_t rc, padding_ui8_t pad) {
    rect_ui16_t rect = { rc.x + pad.left, rc.y + pad.top, 0, 0 };
    if (rc.w > (pad.left + pad.right))
        rect.w = rc.w - (pad.left + pad.right);
    if (rc.h > (pad.top + pad.bottom))
        rect.h = rc.h - (pad.top + pad.bottom);
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
            rect.x = MAX(0, rc.x - (rc1.w - rc.w) / 2);
        break;
    }

    switch (align & ALIGN_VMASK) {
    case ALIGN_TOP:
        rect.y = rc.y;
        break;
    case ALIGN_BOTTOM:
        rect.y = ((rc.y + rc.h) > rc1.h) ? ((rc.y + rc.h) - rc1.h) : 0;
        rect.y = MAX(0, (rc.y + rc.h) - rc1.h);
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
    int len = strlen(str);
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
    return point_ui16((uint16_t)MAX(x, w), (uint16_t)h);
}

int font_line_chars(const font_t *pf, const char *str, uint16_t line_width) {
    int w = 0;
    const int char_w = pf->w;
    int len = strlen(str);
    int n = 0;
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
    return MIN(n, len);
}

point_ui16_t icon_meas(const uint8_t *pi) {
    point_ui16_t wh = { 0, 0 };
    if (memcmp(pi, "\x89PNG", 4) == 0) {
        wh.x = swap_ui16(*((uint16_t *)(pi + 18)));
        wh.y = swap_ui16(*((uint16_t *)(pi + 22)));
    }
    return wh;
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
