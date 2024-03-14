/**
 * @file
 */
#pragma once

#include "stm32f4xx_hal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include "guitypes.hpp"
#include <img_resources.hpp>
#include "Rect16.h"
#include "guiconfig.h"
#include "display_math_helper.h"

// public flags (config)
enum {
    ST7789V_FLG_DMA = 0x08, // DMA enabled
    ST7789V_FLG_MISO = 0x10, // MISO enabled
    ST7789V_FLG_SAFE = 0x20, // SAFE mode (no DMA and safe delay)

    ST7789V_DEF_COLMOD = 0x05, // interface pixel format (5-6-5, hi-color)
    ST7789V_DEF_MADCTL = 0xC0, // memory data access control (mirror XY)

    ST7789V_COLS = 240,
    ST7789V_ROWS = 320,
    ST7789V_BUFF_ROWS = 3,
};

typedef struct _st7789v_config_t {
    SPI_HandleTypeDef *phspi; // spi handle pointer
    uint8_t flg; // flags (DMA, MISO)
    uint8_t colmod; // interface pixel format
    uint8_t madctl; // memory data access control

    uint8_t gamma;
    uint8_t brightness;
    uint8_t is_inverted;
    uint8_t control;
} st7789v_config_t;

inline constexpr uint8_t ST7789V_MAX_COMMAND_READ_LENGHT = 4;

inline uint16_t color_to_565(uint32_t clr) {
    return swap_ui16(((clr >> 19) & 0x001f) | ((clr >> 5) & 0x07e0) | ((clr << 8) & 0xf800));
}

inline uint32_t color_from_565([[maybe_unused]] uint16_t clr565) {
    // TODO
    return 0;
}

extern void st7789v_init(void);
extern void st7789v_done(void);
extern void st7789v_clear(uint16_t clr565);
extern void st7789v_wr(uint8_t *pdata, uint16_t size);
extern void st7789v_fill_rect_colorFormat565(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint16_t clr565);

/**
 * @brief Draw QOI image from file.
 * @param pf pointer to file, it has to be open and pointing to the qoi header
 * @param point_x x coordinate of the top left corner of the image
 * @param point_y y coordinate of the top left corner of the image
 * @param back_color background color, (back_color >> 16) & 0xff is blue, (back_color >> 8) & 0xff is green, back_color & 0xff is red
 * @param rop raster operations as defined in display_math_helper.h and qoi_decoder.h
 * @param subrect subrectangle of the image to draw
 */
void st7789v_draw_qoi_ex(FILE *pf, uint16_t point_x, uint16_t point_y, uint32_t back_color, uint8_t rop, Rect16 subrect);

inline void st7789v_set_backlight([[maybe_unused]] uint8_t bck) {}

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

/**
 * @brief Borrow display buffer.
 * @note This can be used only from the gui thread.
 * @note The buffer needs to be returned before it is used by display.
 * @return Pointer to display buffer.
 */
uint8_t *st7789v_borrow_buffer();

/**
 * @brief Return display buffer so it can be used by display or someone else.
 */
void st7789v_return_buffer();

/**
 * @brief Get size of display buffer.
 * @return size of display buffer in bytes
 */
size_t st7789v_buffer_size();

extern uint16_t st7789v_reset_delay;

extern void st7789v_enable_safe_mode(void);
void st7789v_draw_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

extern void st7789v_spi_tx_complete(void);
extern void st7789v_cmd_madctlrd(uint8_t *pdata);
extern void st7789v_reset(void);
