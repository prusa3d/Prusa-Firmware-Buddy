#pragma once

#include <common/abstract_byte_reader.hpp>
#include <inttypes.h>
#include "guitypes.hpp"
#include "Rect16.h"
#include "fonts.hpp"
#include "raster_opfn.hpp"

namespace display {

/// Enable global resource file to be used.
void enable_resource_file();

void clear(Color clr);
void draw_char(point_ui16_t pt, unichar c, const font_t *pf, Color clr_bg, Color clr_fg);
void draw_char_part(point_ui16_t pt, unichar c, const font_t *pf, Color clr_bg, Color clr_fg, uint16_t start_x, uint16_t end_x);
void draw_img(point_ui16_t pt, const img::Resource &img, Color back_color, ropfn rop);
void draw_img(point_ui16_t pt, AbstractByteReader &);
void draw_line(point_ui16_t pt0, point_ui16_t pt1, Color clr);
void draw_rect(Rect16 rc, Color clr);
void draw_rounded_rect(Rect16 rect, Color back, Color front, uint8_t cor_rad, uint8_t cor_flag, Color secondary_col = COLOR_BLACK);
void draw_text(Rect16 rc, const string_view_utf8 &str, const Font font, Color clr_bg, Color clr_fg);
void fill_rect(Rect16 rc, Color clr);

void set_pixel(point_ui16_t pt, Color clr);
uint8_t *get_block(point_ui16_t start, point_ui16_t end);

void enable_safe_mode();
void init();
bool is_reset_required();
void complete_reinit();
void spi_tx_complete();
void spi_rx_complete();

/**
 * @brief Borrow display buffer.
 * @note This can be used only from the gui thread.
 * @note The buffer needs to be returned before it is used by display.
 * @return Pointer to display buffer.
 */
struct BorrowBuffer {
    [[nodiscard]] BorrowBuffer();
    ~BorrowBuffer();
    uint8_t *buffer;
    operator uint8_t *() { return buffer; }
};

uint32_t buffer_pixel_size();
void store_char_in_buffer(uint16_t char_cnt, uint16_t curr_char_idx, unichar c, const font_t *pf, Color clr_bg, Color clr_fg);
// Stores horizontal part of the character. Can be used for horizontal scrolling.
// \param end_x is the last pixel copied.
// \returns width of the character drawn. Use it for drawing.
uint16_t store_char_part_in_buffer(unichar c, const font_t *pf, Color clr_bg, Color clr_fg, uint16_t start_x, uint16_t end_x);
void draw_from_buffer(point_ui16_t pt, uint16_t w, uint16_t h);
void draw_from_buffer_part(point_ui16_t pt, uint16_t h, uint16_t start_x, uint16_t end_x);

} // namespace display
