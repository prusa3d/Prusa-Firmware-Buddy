//guitypes.cpp

#include "guitypes.hpp"
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
