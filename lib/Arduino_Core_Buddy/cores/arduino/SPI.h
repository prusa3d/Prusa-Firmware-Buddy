// SPI.h
#ifndef _SPI_H
#define _SPI_H

#include "Arduino.h"
#include <stdio.h>

// SPI_HAS_TRANSACTION means SPI has
//   - beginTransaction()
//   - endTransaction()
//   - usingInterrupt()
//   - SPISetting(clock, bitOrder, dataMode)
#define SPI_HAS_TRANSACTION 1

// Compatibility with sketches designed for AVR @ 16 MHz could not
// be ensured as SPI frequency depends of system clock configuration.
// user have to use appropriate divider for the SPI clock
// This function should not be used in new project.
// Use SPISettings with SPI.beginTransaction() to configure SPI parameters.
#define SPI_CLOCK_DIV2   2
#define SPI_CLOCK_DIV4   4
#define SPI_CLOCK_DIV8   8
#define SPI_CLOCK_DIV16  16
#define SPI_CLOCK_DIV32  32
#define SPI_CLOCK_DIV64  64
#define SPI_CLOCK_DIV128 128

// SPI mode parameters for SPISettings
#define SPI_MODE0 0x00
#define SPI_MODE1 0x01
#define SPI_MODE2 0x02
#define SPI_MODE3 0x03

// Transfer mode
enum SPITransferMode {
    SPI_CONTINUE, /* Transfer not finished: CS pin kept active */
    SPI_LAST /* Transfer ended: CS pin released */
};

// Indicates the user controls himself the CS pin outside of the spi class
#define CS_PIN_CONTROLLED_BY_USER NUM_DIGITAL_PINS

// Indicates there is no configuration selected
#define NO_CONFIG ((int16_t)(-1))

// Defines a default timeout delay in milliseconds for the SPI transfer
#define SPI_TRANSFER_TIMEOUT 1000

/*
 * Defines the number of settings saved per SPI instance. Must be in range 1 to 254.
 * Can be redefined in variant.h
 */
#ifndef NB_SPI_SETTINGS
    #define NB_SPI_SETTINGS 4
#endif

class SPISettings {
public:
    SPISettings();
    SPISettings(uint32_t clock, BitOrder bitOrder, uint8_t dataMode);
};

class SPIClass {
public:
    SPIClass();
    void begin(uint8_t _pin = CS_PIN_CONTROLLED_BY_USER);
    void end(void);
    void beginTransaction(SPISettings settings);
    void endTransaction(void);
    byte transfer(uint8_t _data, SPITransferMode _mode = SPI_LAST);
    uint16_t transfer16(uint16_t _data, SPITransferMode _mode = SPI_LAST);
    uint8_t dmaTransfer(uint8_t *transmitBuf, uint8_t *receiveBuf, uint16_t length);
    uint8_t dmaSend(uint8_t *buf, uint16_t length);
};

extern SPIClass SPI;

#endif //_SPI_H
