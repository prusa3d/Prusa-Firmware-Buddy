// guitypes.cpp

#include "guitypes.hpp"
#include <algorithm>

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
