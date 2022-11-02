//guitypes.cpp

#include "guitypes.hpp"
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

/// TODO refactor to icon_size
point_ui16_t icon_meas(const uint8_t *pi) {
    point_ui16_t wh = { 0, 0 };
    if (memcmp(pi, "\x89PNG", 4) == 0) {
        wh.x = __builtin_bswap16(*((uint16_t *)(pi + 18)));
        wh.y = __builtin_bswap16(*((uint16_t *)(pi + 22)));
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

const uint8_t *resource_ptr(ResourceId id) {
    if (id < resource_count)
        return resource_table[id].ptr;
    return 0;
}

uint16_t resource_size(ResourceId id) {
    if (id < resource_count)
        return resource_table[id].size;
    return 0;
}

FILE *resource_fopen(ResourceId id, const char *opentype) {
    if (id < resource_count)
        return fmemopen((uint8_t *)resource_table[id].ptr, resource_table[id].size, opentype);
    return 0;
}

font_t *resource_font(ResourceId id) {
    if (id < resource_count)
        return (font_t *)resource_table[id].ptr;
    return 0;
}
