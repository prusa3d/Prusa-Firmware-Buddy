//display_math_helper.h
//basic mathematical operations for display
#pragma once

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

__attribute__((used)) inline uint16_t swap_ui16(uint16_t val) {
    return __builtin_bswap16(val);
    //return (val >> 8) | ((val & 0xff) << 8);
}

__attribute__((used)) inline uint32_t color_rgb(const uint8_t r, const uint8_t g, const uint8_t b) {
    return r | ((uint32_t)g << 8) | ((uint32_t)b << 16);
}

__attribute__((used)) inline uint32_t color_alpha(const uint32_t clr0, const uint32_t clr1, const uint8_t alpha) {
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

#ifdef __cplusplus
}
#endif //__cplusplus
