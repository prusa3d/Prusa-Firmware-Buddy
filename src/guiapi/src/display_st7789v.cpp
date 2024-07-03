#include "display.hpp"

#include "st7789v.hpp"

namespace display {

BorrowBuffer::BorrowBuffer()
    : buffer { st7789v_borrow_buffer() } {
}

BorrowBuffer::~BorrowBuffer() {
    st7789v_return_buffer();
}

uint32_t buffer_pixel_size() {
    return st7789v_buffer_size() / 2;
}

void enable_safe_mode() {
    st7789v_enable_safe_mode();
}

void init() {
    st7789v_init();
}

uint8_t *get_block(point_ui16_t start, point_ui16_t end) {
    return st7789v_get_block(start.x, start.y, end.x, end.y);
}

bool is_reset_required() {
    return st7789v_is_reset_required();
}

void complete_reinit() {
    st7789v_reset();
}

void spi_tx_complete() {
    st7789v_spi_tx_complete();
}

void spi_rx_complete() {
    // We are not using this
}

} // namespace display
