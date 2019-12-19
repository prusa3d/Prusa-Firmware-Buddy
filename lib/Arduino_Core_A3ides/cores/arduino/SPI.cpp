//SPI.cpp
#include "SPI.h"
//#include <spi_com.h>
#include "stm32f4xx_hal.h"
//#include <Arduino.h>

extern SPI_HandleTypeDef hspi1;
extern int HAL_SPI_Initialized;

typedef enum {
    SPI_OK = 0,
    SPI_TIMEOUT = 1,
    SPI_ERROR = 2,
    SPI_NOT_INITIALIZED = 3,
} spi_status_e;

//transmit function
spi_status_e _spi_transfer(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t len, uint32_t Timeout) {
    if (HAL_SPI_Initialized) {
        HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(&hspi1, tx_buffer, rx_buffer, len, Timeout);
        if (ret == HAL_TIMEOUT)
            return SPI_TIMEOUT;
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

// following six methods is empty because all configuration is done with Cube
SPISettings::SPISettings() {}
SPISettings::SPISettings(uint32_t clock, BitOrder bitOrder, uint8_t dataMode) {}
SPIClass::SPIClass() {}
void SPIClass::begin(uint8_t _pin) {}
void SPIClass::end(void) {}
void SPIClass::beginTransaction(SPISettings settings) {}
void SPIClass::endTransaction(void) {}

//8bit transfer
byte SPIClass::transfer(uint8_t _data, SPITransferMode _mode) {
    uint8_t read;
    dmaTransfer(&_data, &read, sizeof(_data));
    return read;
}

//16bit transfer
uint16_t SPIClass::transfer16(uint16_t _data, SPITransferMode _mode) {
    uint16_t read;
    dmaTransfer((uint8_t *)&_data, (uint8_t *)&read, sizeof(_data));
    return read;
}

uint8_t SPIClass::dmaTransfer(uint8_t *transmitBuf, uint8_t *receiveBuf, uint16_t length) {
    if (!HAL_SPI_Initialized)
        return SPI_NOT_INITIALIZED;

    auto status = HAL_SPI_TransmitReceive(&hspi1, transmitBuf, receiveBuf, length, SPI_TRANSFER_TIMEOUT);
    return hal_status_to_retval(status);
}

uint8_t SPIClass::dmaSend(uint8_t *buf, uint16_t length) {
    if (!HAL_SPI_Initialized)
        return SPI_NOT_INITIALIZED;

    auto status = HAL_SPI_Transmit(&hspi1, buf, length, SPI_TRANSFER_TIMEOUT);
    return hal_status_to_retval(status);
}

SPIClass SPI = SPIClass();
