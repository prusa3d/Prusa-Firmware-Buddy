#include "i2c.h"
#include "bsod.h"

#define EEPROM_MAX_RETRIES 10

volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_HAL_OK = 0;
volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_HAL_ERROR = 0;
volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_HAL_BUSY = 0;
volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_HAL_TIMEOUT = 0;
volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_UNDEF = 0;

extern "C" void I2C_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {

    int retries = EEPROM_MAX_RETRIES;
    HAL_StatusTypeDef result = HAL_ERROR;
    while (--retries) {
        result = HAL_I2C_Master_Transmit(hi2c, DevAddress, pData, Size, Timeout);
        if (result != HAL_BUSY)
            break;
    }

    switch (result) {
    case HAL_OK:
        ++I2C_TRANSMIT_RESULTS_HAL_OK;
        break;
    case HAL_ERROR:
        ++I2C_TRANSMIT_RESULTS_HAL_ERROR;
        general_error("EEPROM Transmit", "I2C error");
        break;
    case HAL_BUSY:
        ++I2C_TRANSMIT_RESULTS_HAL_BUSY;
        general_error("EEPROM Transmit", "I2C busy");
        break;
    case HAL_TIMEOUT:
        ++I2C_TRANSMIT_RESULTS_HAL_TIMEOUT;
        general_error("EEPROM Transmit", "I2C timeout");
        break;
    default:
        ++I2C_TRANSMIT_RESULTS_UNDEF;
        general_error("EEPROM Transmit", "I2C undefined error");
        break;
    }
}

volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_HAL_OK = 0;
volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_HAL_ERROR = 0;
volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_HAL_BUSY = 0;
volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_HAL_TIMEOUT = 0;
volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_UNDEF = 0;

extern "C" void I2C_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {

    int retries = EEPROM_MAX_RETRIES;
    HAL_StatusTypeDef result = HAL_ERROR;
    while (--retries) {
        result = HAL_I2C_Master_Receive(hi2c, DevAddress, pData, Size, Timeout);
        if (result != HAL_BUSY)
            break;
    }

    switch (result) {
    case HAL_OK:
        ++I2C_RECEIVE_RESULTS_HAL_OK;
        break;
    case HAL_ERROR:
        ++I2C_RECEIVE_RESULTS_HAL_ERROR;
        general_error("EEPROM Receive", "I2C error");
        break;
    case HAL_BUSY:
        ++I2C_RECEIVE_RESULTS_HAL_BUSY;
        general_error("EEPROM Receive", "I2C busy");
        break;
    case HAL_TIMEOUT:
        ++I2C_RECEIVE_RESULTS_HAL_TIMEOUT;
        general_error("EEPROM Receive", "I2C timeout");
        break;
    default:
        ++I2C_RECEIVE_RESULTS_UNDEF;
        general_error("EEPROM Receive", "I2C undefined error");
        break;
    }
}
