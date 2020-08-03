//st7789v.h
#pragma once

#include "stm32f4xx_hal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include "guitypes.h"

//public flags (config)
#define ST7789V_FLG_DMA  0x08 // DMA enabled
#define ST7789V_FLG_MISO 0x10 // MISO enabled
#define ST7789V_FLG_SAFE 0x20 // SAFE mode (no DMA and safe delay)

#define ST7789V_DEF_COLMOD 0x05 // interface pixel format (5-6-5, hi-color)
#define ST7789V_DEF_MADCTL 0xC0 // memory data access control (mirror XY)

#define ST7789V_COLS 240 //
#define ST7789V_ROWS 320 //

typedef struct _st7789v_config_t {
    SPI_HandleTypeDef *phspi; // spi handle pointer
    uint8_t pinCS;            // CS pin
    uint8_t pinRS;            // RS pin
    uint8_t pinRST;           // RST pin
    uint8_t flg;              // flags (DMA, MISO)
    uint8_t colmod;           // interface pixel format
    uint8_t madctl;           // memory data access control

    uint8_t gamma;
    uint8_t brightness;
    uint8_t is_inverted;
    uint8_t control;
} st7789v_config_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

__attribute__((used)) inline uint16_t swap_ui16(uint16_t val) {
    return __builtin_bswap16(val);
    //return (val >> 8) | ((val & 0xff) << 8);
}

__attribute__((used)) inline uint16_t color_to_565(uint32_t clr) {
    return swap_ui16(((clr >> 19) & 0x001f) | ((clr >> 5) & 0x07e0) | ((clr << 8) & 0xf800));
}

__attribute__((used)) inline uint32_t color_rgb(const uint8_t r, const uint8_t g, const uint8_t b) {
    return r | ((uint32_t)g << 8) | ((uint32_t)b << 16);
}

__attribute__((used)) inline uint32_t color_from_565(uint16_t clr565) {
    //TODO
    return 0;
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

extern void png_meas(void);
extern void st7789v_init(void);
extern void st7789v_done(void);
extern void st7789v_clear_C(uint16_t clr565);
extern void st7789v_set_pixel_C(uint16_t point_x, uint16_t point_y, uint16_t clr565);
extern void st7789v_fill_rect_C(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint16_t clr565);

extern void st7789v_draw_png_ex(uint16_t point_x, uint16_t point_y, FILE *pf, uint32_t clr0, uint8_t rop);

extern void st7789v_inversion_on(void);
extern void st7789v_inversion_off(void);
extern void st7789v_inversion_tgl(void);
extern uint8_t st7789v_inversion_get(void);

extern void st7789v_gamma_next(void);
extern void st7789v_gamma_prev(void);
extern void st7789v_gamma_set(uint8_t gamma);
extern uint8_t st7789v_gamma_get(void);

extern void st7789v_brightness_set(uint8_t brightness);
extern uint8_t st7789v_brightness_get(void);

extern void st7789v_brightness_enable(void);
extern void st7789v_brightness_disable(void);

extern uint32_t st7789v_get_pixel_C(uint16_t point_x, uint16_t point_y);
extern void st7789v_set_pixel_directColor_C(uint16_t point_x, uint16_t point_y, uint16_t noClr);
extern uint16_t st7789v_get_pixel_directColor_C(uint16_t point_x, uint16_t point_y);

extern st7789v_config_t st7789v_config;

extern uint16_t st7789v_reset_delay;

extern void st7789v_enable_safe_mode(void);

extern void st7789v_spi_tx_complete(void);

#ifdef __cplusplus
}
#endif //__cplusplus
