#pragma once

#include "display_math_helper.h"
#include "Rect16.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

enum {
    ST7789V_COLS = 240,
    ST7789V_ROWS = 320,
    ST7789V_BUFF_ROWS = 3,
    ST7789V_BUFFER_SIZE = ST7789V_COLS * ST7789V_BUFF_ROWS * 2
};

inline uint16_t color_to_565(Color clr) {
    static constexpr uint8_t r_shift = (8 - 5);
    static constexpr uint8_t g_shift = (8 - 6) + r_shift;
    static constexpr uint8_t b_shift = (8 - 5) + g_shift;
    return swap_ui16(((clr.raw >> r_shift) & 0x001f) | ((clr.raw >> g_shift) & 0x07e0) | ((clr.raw >> b_shift) & 0xf800));
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
void st7789v_draw_qoi_ex(FILE *pf, uint16_t point_x, uint16_t point_y, Color back_color, uint8_t rop, Rect16 subrect);

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
extern bool st7789v_is_reset_required();
extern void st7789v_reset(void);
