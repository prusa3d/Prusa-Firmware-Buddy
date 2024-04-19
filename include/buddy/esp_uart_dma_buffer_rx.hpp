#pragma once

#include <cstdint>

// UART buffer stuff
// The data received should fit into the buffer. Or, some guaratees has to be
// provided to ensure the excessive data can be copied from the RX buffer
// before the buffer overflows.
#define RX_BUFFER_LEN 0x1000

extern uint8_t dma_buffer_rx[RX_BUFFER_LEN];
