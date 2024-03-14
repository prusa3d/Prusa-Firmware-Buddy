/// @file usart.cpp
#include "../usart.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

namespace hal {
namespace usart {

USART usart1(USART1);

uint8_t USART::Read() {
    uint8_t c = 0;
    rx_buf.pop(c);
    return c;
}

void USART::Write(uint8_t c) {
    _written = true;
    // If the buffer and the data register is empty, just write the byte
    // to the data register and be done. This shortcut helps
    // significantly improve the effective datarate at high (>
    // 500kbit/s) bitrates, where interrupt overhead becomes a slowdown.
    if (tx_buf.empty() && (husart->UCSRxA & (1 << 5))) {
        husart->UDRx = c;
        husart->UCSRxA |= (1 << 6);
        return;
    }

    // If the output buffer is full, there's nothing for it other than to
    // wait for the interrupt handler to empty it a bit
    while (!tx_buf.push(c)) {
        if (bit_is_clear(SREG, SREG_I)) {
            // Interrupts are disabled, so we'll have to poll the data
            // register empty flag ourselves. If it is set, pretend an
            // interrupt has happened and call the handler to free up
            // space for us.
            if (husart->UCSRxA & (1 << 5)) {
                ISR_UDRE();
            }
        } else {
            // nop, the interrupt handler will free up space for us
        }
    }

    husart->UCSRxB |= (1 << 5); //enable UDRE interrupt
}

void USART::Flush() {
    // If we have never written a byte, no need to flush. This special
    // case is needed since there is no way to force the TXC (transmit
    // complete) bit to 1 during initialization
    if (!_written) {
        return;
    }

    while ((husart->UCSRxB & (1 << 5)) || ~(husart->UCSRxA & (1 << 6))) {
        if (bit_is_clear(SREG, SREG_I) && (husart->UCSRxB & (1 << 5)))
            // Interrupts are globally disabled, but the DR empty
            // interrupt should be enabled, so poll the DR empty flag to
            // prevent deadlock
            if (husart->UCSRxA & (1 << 5)) {
                ISR_UDRE();
            }
    }
    // If we get here, nothing is queued anymore (DRIE is disabled) and
    // the hardware finished tranmission (TXC is set).
}

void USART::WriteS(const char *str) {
    while (*str != 0) {
        Write(*str++);
    }
}

void USART::WriteS_P(const char *str_P) {
    char c = 0;
    while ((c = pgm_read_byte(str_P++)) != 0) {
        Write(c);
    }
}

void USART::puts(const char *str) {
    WriteS(str);
    Write('\n');
}

void USART::puts_P(const char *str_P) {
    WriteS_P(str_P);
    Write('\n');
}

} // namespace usart
} // namespace hal

ISR(USART1_RX_vect) {
    hu::usart1.ISR_RX();
}

ISR(USART1_UDRE_vect) {
    hu::usart1.ISR_UDRE();
}
