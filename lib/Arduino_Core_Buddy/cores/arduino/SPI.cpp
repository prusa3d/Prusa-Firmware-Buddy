#include <device/board.h>

#if (BOARD_IS_XLBUDDY() || BOARD_IS_XBUDDY())
    #include "SPI.h"
    #include <device/board.h>
    #include <device/hal.h>
    #include <device/peripherals.h>

extern int HAL_SPI_Initialized;

typedef enum {
    SPI_OK = 0,
    SPI_TIMEOUT = 1,
    SPI_ERROR = 2,
    SPI_NOT_INITIALIZED = 3,
} spi_status_e;

// transmit function
spi_status_e _spi_transfer(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t len, uint32_t Timeout) {
    if (HAL_SPI_Initialized) {
        HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(&SPI_HANDLE_FOR(tmc), tx_buffer, rx_buffer, len, Timeout);
        if (ret == HAL_TIMEOUT) {
            return SPI_TIMEOUT;
        }
        return SPI_OK;
    }
    return SPI_ERROR;
}

static uint16_t hal_status_to_retval(HAL_StatusTypeDef status) {
    switch (status) {
    case HAL_OK:
        return SPI_OK;
    case HAL_TIMEOUT:
        return SPI_TIMEOUT;
    default:
        return SPI_ERROR;
    }
}

// following six methods is empty because all configuration is done with stm32cube
SPISettings::SPISettings() {}
SPISettings::SPISettings(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) {}
SPIClass::SPIClass() {}
void SPIClass::begin(uint8_t _pin) {}
void SPIClass::end(void) {}
void SPIClass::beginTransaction(SPISettings settings) {}
void SPIClass::endTransaction(void) {}

// 8bit transfer
byte SPIClass::transfer(uint8_t _data, SPITransferMode _mode) {
    uint8_t read;
    dmaTransfer(&_data, &read, sizeof(_data));
    return read;
}

/**
 * @brief  Transfer two bytes of data.
 *
 * Arduino is sending the most significant byte first,
 * if the SPI interface is configured to send the most significant bit first.
 * If the least significant bit is send first,
 * Arduino will also send the least significant byte first.
 *
 * This wrapper currently only supports sending the most significant bit first over the wire,
 * so this function will also always send the most significant byte first.
 *
 * @param [in]data The two bytes to send
 * @return The received two bytes
 */

uint16_t SPIClass::transfer16(uint16_t _data, SPITransferMode _mode) {
    _data = ((_data << 8) & 0xff00) | ((_data >> 8) & 0x00ff);
    uint16_t read;
    dmaTransfer((uint8_t *)&_data, (uint8_t *)&read, sizeof(_data));
    read = ((read << 8) & 0xff00) | ((read >> 8) & 0x00ff);
    return read;
}

uint8_t SPIClass::dmaTransfer(uint8_t *transmitBuf, uint8_t *receiveBuf, uint16_t length) {
    if (!HAL_SPI_Initialized) {
        return SPI_NOT_INITIALIZED;
    }

    auto status = HAL_SPI_TransmitReceive(&SPI_HANDLE_FOR(tmc), transmitBuf, receiveBuf, length, SPI_TRANSFER_TIMEOUT);
    return hal_status_to_retval(status);
}

uint8_t SPIClass::dmaSend(uint8_t *buf, uint16_t length) {
    if (!HAL_SPI_Initialized) {
        return SPI_NOT_INITIALIZED;
    }

    auto status = HAL_SPI_Transmit(&SPI_HANDLE_FOR(tmc), buf, length, SPI_TRANSFER_TIMEOUT);
    return hal_status_to_retval(status);
}

SPIClass SPI = SPIClass();
#endif
