/**
 * @file display_math_helper.h
 * @brief basic mathematical operations for display
 */
#pragma once

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

__attribute__((used)) inline uint16_t swap_ui16(uint16_t val) {
    return __builtin_bswap16(val);
    // return (val >> 8) | ((val & 0xff) << 8);
}

__attribute__((used)) inline uint32_t color_rgb(const uint8_t r, const uint8_t g, const uint8_t b) {
    // saved in bgr format, because ARM is little endian and when it is saved to memory it will be in rgb order
    return r | ((uint32_t)g << 8) | ((uint32_t)b << 16);
}

__attribute__((used)) inline uint32_t color_alpha(const uint32_t clr0, const uint32_t clr1, const uint8_t alpha) {
    // when color is saved in uint32_t it is in bgr format, because ARM uses little endian
    const uint8_t r0 = clr0 & 0xff;
    const uint8_t g0 = (clr0 >> 8) & 0xff;
    const uint8_t b0 = (clr0 >> 16) & 0xff;
    const uint8_t r1 = clr1 & 0xff;
    const uint8_t g1 = (clr1 >> 8) & 0xff;
    const uint8_t b1 = (clr1 >> 16) & 0xff;
    const uint8_t r = ((255 - alpha) * r0 + alpha * r1) / 255;
    const uint8_t g = ((255 - alpha) * g0 + alpha * g1) / 255;
    const uint8_t b = ((255 - alpha) * b0 + alpha * b1) / 255;
    return color_rgb(r, g, b);
}

__attribute__((used)) inline void rop_rgb888_invert(uint8_t *ppx888) {
    ppx888[0] = 255 - ppx888[0];
    ppx888[1] = 255 - ppx888[1];
    ppx888[2] = 255 - ppx888[2];
}

static const uint8_t SWAPBW_TOLERANCE = 64;

__attribute__((used)) static inline void
rop_rgb888_swapbw(uint8_t *ppx888) {
    const uint8_t r = ppx888[0];
    const uint8_t g = ppx888[1];
    const uint8_t b = ppx888[2];
    const uint8_t l = (r + g + b) / 3;
    const uint8_t l0 = (l >= SWAPBW_TOLERANCE) ? (l - SWAPBW_TOLERANCE) : 0;
    const uint8_t l1 = (l <= (255 - SWAPBW_TOLERANCE)) ? (l + SWAPBW_TOLERANCE) : 255;

    if ((l0 <= r) && (r <= l1) && (l0 <= g) && (g <= l1) && (l0 <= b) && (b <= l1)) {
        ppx888[0] = 255 - r;
        ppx888[1] = 255 - g;
        ppx888[2] = 255 - b;
    }
}

__attribute__((used)) static inline void rop_rgb888_disabled(uint8_t *ppx888) {
    const uint8_t r = ppx888[0];
    const uint8_t g = ppx888[1];
    const uint8_t b = ppx888[2];
    const uint8_t l = (r + g + b) / 3;
    const uint8_t l0 = (l >= SWAPBW_TOLERANCE) ? (l - SWAPBW_TOLERANCE) : 0;
    const uint8_t l1 = (l <= (255 - SWAPBW_TOLERANCE)) ? (l + SWAPBW_TOLERANCE) : 255;
    if ((l0 <= r) && (r <= l1) && (l0 <= g) && (g <= l1) && (l0 <= b) && (b <= l1)) {
        ppx888[0] = r / 2;
        ppx888[1] = g / 2;
        ppx888[2] = b / 2;
    }
}

__attribute__((used)) static inline void rop_rgb888_desaturate(uint8_t *ppx888) {
    const uint8_t avg = (ppx888[0] + ppx888[1] + ppx888[2]) / 3;
    ppx888[0] = avg;
    ppx888[1] = avg;
    ppx888[2] = avg;
}

__attribute__((used)) static inline void rop_rgb8888_swapbw(uint8_t *ppx) {
    const uint8_t r = ppx[0];
    const uint8_t g = ppx[1];
    const uint8_t b = ppx[2];
    const uint8_t a = ppx[3];
    if ((r == g) && (r == b)) {
        ppx[0] = 255 - r;
        ppx[1] = 255 - g;
        ppx[2] = 255 - b;
        if (/*(r < 32) && */ (a < 128)) {
            ppx[0] = 0;
            ppx[1] = 0;
            ppx[2] = 255;
            ppx[3] = 255;
        }
    } else if (a < 128) {
        ppx[0] = 0;
        ppx[1] = 255;
        ppx[2] = 0;
        ppx[3] = 255;
    }
}

#ifdef __cplusplus
}
#endif //__cplusplus
