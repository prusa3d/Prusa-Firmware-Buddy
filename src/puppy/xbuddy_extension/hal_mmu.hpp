/// @file
#pragma once

#include <cstddef>
#include <cstdint>
#include <utils/byte_utils.hpp>

#include <option/has_mmu2.h>
static_assert(HAS_MMU2());

namespace hal::mmu {

/// Initialize MMU UART and pins.
void init();

/// MSP Initialization for MMU UART, for internal use only.
void msp_init(void *);

/// RX callback, for internal use only.
void rx_callback(void *, uint16_t size);

/// TX callback, for internal use only.
void tx_callback(void *);

/// Transmit bytes on MMU UART.
/// Blocks until all bytes are transmitted.
void transmit(Bytes);

/// Receive bytes from MMU UART.
/// Bytes are received into supplied buffer.
/// Returns view into that buffer.
/// Does not block.
WritableBytes receive(WritableBytes);

/// Flush the receive buffer, discarding its contents.
void flush();

} // namespace hal::mmu
