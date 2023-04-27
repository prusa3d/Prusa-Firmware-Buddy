/// @file usart.h
#pragma once
#include <inttypes.h>
#include <avr/io.h>
#include "gpio.h"
#include "circular_buffer.h"

namespace hal {

/// USART interface
/// @@TODO decide, if this class will behave like a singleton, or there will be multiple classes
/// for >1 USART interfaces
namespace usart {

constexpr uint16_t UART_BAUD_SELECT(uint32_t baudRate, uint32_t xtalCpu) {
    return (((double)(xtalCpu)) / (((double)(baudRate)) * 8.0) - 1.0 + 0.5);
}

class USART {
public:
    struct USART_TypeDef {
        volatile uint8_t UCSRxA;
        volatile uint8_t UCSRxB;
        volatile uint8_t UCSRxC;
        volatile uint8_t UCSRxD;
        volatile uint16_t UBRRx;
        volatile uint8_t UDRx;
    };

    struct USART_InitTypeDef {
        hal::gpio::GPIO_pin rx_pin;
        hal::gpio::GPIO_pin tx_pin;
        uint32_t baudrate;
    };

    /// @returns current character from the UART without extracting it from the read buffer
    uint8_t Peek() const {
        return rx_buf.front();
    }
    /// @returns true if there are no bytes to be read
    bool ReadEmpty() const {
        return rx_buf.empty();
    }
    /// @returns current character from the UART and extracts it from the read buffer
    uint8_t Read();

    /// @param c character to be pushed into the TX buffer (to be sent)
    void Write(uint8_t c);
    /// @param str pointer to a string in RAM to be pushed byte-by-byte into the TX buffer (to be sent)
    /// No NL character is appended
    void WriteS(const char *str);
    /// @param str_P pointer to a string in PROGMEM to be pushed byte-by-byte into the TX buffer (to be sent)
    /// No NL character is appended
    void WriteS_P(const char *str_P);

    /// @param str c string to be sent. NL is appended
    /// Works on RAM strings
    void puts(const char *str);
    /// @param str_P c string to be sent. NL is appended
    /// Works on PROGMEM strings
    void puts_P(const char *str_P);
    /// @returns true if there is at least one byte free in the TX buffer (i.e. some space to add a character to be sent)
    bool CanWrite() const {
        return !tx_buf.full();
    }
    /// blocks until the TX buffer was successfully transmitted
    void Flush();

    /// Enable the RX receiver
    __attribute__((always_inline)) inline void rx_enable() { husart->UCSRxB |= (1 << RXEN1); };

    /// Initializes USART interface
    __attribute__((always_inline)) inline void Init(USART_InitTypeDef *const conf) {
        gpio::Init(conf->rx_pin, gpio::GPIO_InitTypeDef(gpio::Mode::input, gpio::Level::low));
        gpio::Init(conf->tx_pin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::low));
        husart->UBRRx = UART_BAUD_SELECT(conf->baudrate, F_CPU);
        husart->UCSRxA |= (1 << U2X1); // Set double baudrate setting. Clear all other status bits/flags
        // husart->UCSRxC |= (1 << 3); // 2 stop bits. Preserve data size setting
        husart->UCSRxD = 0; // disable hardware flow control. Few avr MCUs have this feature, but this register is reserved on all AVR devices with USART, so we can disable it without consequences.
        husart->UCSRxB = (1 << TXEN1) | (1 << RXCIE1); // Turn on the transmission and reception circuitry and enable the RX interrupt
    }

    /// implementation of the receive ISR's body
    __attribute__((always_inline)) inline void ISR_RX() {
        if (husart->UCSRxA & (1 << FE1)) {
            (void)husart->UDRx;
        } else {
            rx_buf.push((uint8_t)husart->UDRx);
        }
    }
    /// implementation of the transmit ISR's body
    __attribute__((always_inline)) inline void ISR_UDRE() {
        uint8_t c = 0;
        tx_buf.pop(c);
        husart->UDRx = c;

        // clear the TXC bit -- "can be cleared by writing a one to its bit
        // location". This makes sure flush() won't return until the bytes
        // actually got written
        husart->UCSRxA |= (1 << TXC1);

        if (tx_buf.empty())
            husart->UCSRxB &= ~(1 << UDRIE1); // disable UDRE interrupt
    }

    USART(USART_TypeDef *husart)
        : husart(husart) {};

private:
    // IO base address
    USART_TypeDef *husart;
    bool _written;

    CircularBuffer<uint8_t, uint_fast8_t, 32> tx_buf;
    CircularBuffer<uint8_t, uint_fast8_t, 32> rx_buf;
};

/// beware - normally we'd make a singleton, but avr-gcc generates suboptimal code for them, therefore we only keep this extern variable
extern USART usart1;

} // namespace usart
} // namespace hal

#define USART1 ((hal::usart::USART::USART_TypeDef *)&UCSR1A)

namespace hu = hal::usart;
