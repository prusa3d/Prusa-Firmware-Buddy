/**
 * @file
 */
#pragma once

#include "stm32f4xx_hal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include "guitypes.h"
#include "display_math_helper.h"

//public flags (config)
enum {
    ST7789V_FLG_DMA = 0x08,  // DMA enabled
    ST7789V_FLG_MISO = 0x10, // MISO enabled
    ST7789V_FLG_SAFE = 0x20, // SAFE mode (no DMA and safe delay)

    ST7789V_DEF_COLMOD = 0x05, // interface pixel format (5-6-5, hi-color)
    ST7789V_DEF_MADCTL = 0xC0, // memory data access control (mirror XY)

    ST7789V_COLS = 240,
    ST7789V_ROWS = 320,
    ST7789V_BUFF_ROWS = 16,
};

typedef struct _st7789v_config_t {
    SPI_HandleTypeDef *phspi; // spi handle pointer
    uint8_t flg;              // flags (DMA, MISO)
    uint8_t colmod;           // interface pixel format
    uint8_t madctl;           // memory data access control

    uint8_t gamma;
    uint8_t brightness;
    uint8_t is_inverted;
    uint8_t control;
} st7789v_config_t;

inline uint16_t color_to_565(uint32_t clr) {
    return swap_ui16(((clr >> 19) & 0x001f) | ((clr >> 5) & 0x07e0) | ((clr << 8) & 0xf800));
}

inline uint32_t color_from_565(uint16_t clr565) {
    //TODO
    return 0;
}

extern void st7789v_init(void);
extern void st7789v_done(void);
extern void st7789v_clear(uint16_t clr565);
extern void st7789v_fill_rect_colorFormat565(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint16_t clr565);

extern void st7789v_draw_png_ex(uint16_t point_x, uint16_t point_y, FILE *pf, uint32_t clr_back, uint8_t rop);

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

extern uint16_t st7789v_get_pixel_colorFormat565(uint16_t point_x, uint16_t point_y);
extern uint8_t *st7789v_get_block(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);
extern void st7789v_set_pixel(uint16_t point_x, uint16_t point_y, uint16_t clr565);

extern st7789v_config_t st7789v_config;

extern uint16_t st7789v_reset_delay;

extern void st7789v_enable_safe_mode(void);

extern void st7789v_spi_tx_complete(void);
