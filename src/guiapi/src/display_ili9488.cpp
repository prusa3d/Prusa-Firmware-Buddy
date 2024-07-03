#include "ili9488.hpp"

#include "display.hpp"

namespace display {

BorrowBuffer::BorrowBuffer()
    : buffer { ili9488_borrow_buffer() } {
}

BorrowBuffer::~BorrowBuffer() {
    ili9488_return_buffer();
}

uint32_t buffer_pixel_size() {
    return ili9488_buffer_size() / 3;
}

//! @brief enable safe mode (direct acces + safe delay)
void enable_safe_mode() {
    ili9488_enable_safe_mode();
}

void init() {
    ili9488_init();
}

uint8_t *get_block(point_ui16_t start, point_ui16_t end) {
    return ili9488_get_block(start.x, start.y, end.x, end.y);
}

bool is_reset_required() {
    return ili9488_is_reset_required();
}

void complete_reinit() {
    ili9488_set_complete_lcd_reinit();
}

void spi_tx_complete() {
    ili9488_spi_tx_complete();
}

void spi_rx_complete() {
    ili9488_spi_rx_complete();
}

} // namespace display
