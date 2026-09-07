/// @file
#pragma once

#include <cstddef>
#include <cstdint>
#include <utils/byte_utils.hpp>

namespace hal::rs485 {

/// Initialize RS485 UART.
void init();

/// MSP Initialization for RS485 UART, for internal use only.
void msp_init(void *);

/// RX callback, for internal use only.
void rx_callback(void *, uint16_t size);

/// TX callback, for internal use only.
void tx_callback(void *);

/// Error callback, for internal use only.
void error_callback(void *);

/// Start receiving messages.
/// Does not block.
void start_receiving();

/// Blocks until message is received.
/// Returned span is valid until next transmit()
WritableBytes receive();

/// Blocks until message is received or timeout occurs.
/// Returned span is valid until next transmit()
/// Returns empty span if timeout occurs.
WritableBytes receive_timeout(uint32_t timeout_ms);

/// Transmit message.
/// Does not block.
/// Supplied span must remain valid until next receive()
void transmit_and_then_start_receiving(WritableBytes);

/// Clear bus errors if needed.
void housekeeping();

/// Block until the MODBUS inter-frame silent interval since last received byte
/// has elapsed. Called by the master before transmitting a response.
void ensure_silent_interval();

} // namespace hal::rs485
